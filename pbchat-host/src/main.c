#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include "pb_decode.h"
#include "pb_encode.h"

#include "proto/chat.pb.h"

#define PROTO_FRAME_HDR_SIZE 2U
#define PROTO_MAX_PAYLOAD    ChatMsg_size

typedef struct
{
    uint16_t expected_len;
    uint16_t received_len;
    uint8_t header_buf[PROTO_FRAME_HDR_SIZE];
    uint8_t payload_buf[PROTO_MAX_PAYLOAD];
} rx_state_t;

static void log_raw_frame(const char *tag, const uint8_t *header, const uint8_t *payload, uint16_t payload_len)
{
    printf("%s len=%u data=", tag, (unsigned)(PROTO_FRAME_HDR_SIZE + payload_len));
    for (uint32_t i = 0; i < PROTO_FRAME_HDR_SIZE; ++i) {
        printf("%02X", header[i]);
        if ((i + 1U) < PROTO_FRAME_HDR_SIZE || payload_len > 0U) {
            printf(" ");
        }
    }
    for (uint16_t i = 0; i < payload_len; ++i) {
        printf("%02X", payload[i]);
        if ((uint16_t)(i + 1U) < payload_len) {
            printf(" ");
        }
    }
    printf("\n");
    fflush(stdout);
}

static int serial_open(const char *dev)
{
    int fd = open(dev, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        return -1;
    }

    struct termios tio;
    if (tcgetattr(fd, &tio) != 0) {
        close(fd);
        return -1;
    }

    cfmakeraw(&tio);
    cfsetispeed(&tio, B115200);
    cfsetospeed(&tio, B115200);
    tio.c_cflag |= (CLOCAL | CREAD);
    tio.c_cflag &= ~CRTSCTS;
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tio) != 0) {
        close(fd);
        return -1;
    }

    return fd;
}

static bool serial_write_all(int fd, const uint8_t *buf, size_t len)
{
    size_t sent = 0U;
    while (sent < len) {
        ssize_t rc = write(fd, buf + sent, len - sent);
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        sent += (size_t)rc;
    }
    return true;
}

static bool send_chat(int fd, const ChatMsg *msg)
{
    uint8_t payload[PROTO_MAX_PAYLOAD];
    pb_ostream_t ostream = pb_ostream_from_buffer(payload, sizeof(payload));
    if (!pb_encode(&ostream, ChatMsg_fields, msg)) {
        return false;
    }

    uint16_t payload_len = (uint16_t)ostream.bytes_written;
    uint8_t header[PROTO_FRAME_HDR_SIZE] = {
        (uint8_t)(payload_len & 0xFFu),
        (uint8_t)((payload_len >> 8) & 0xFFu)
    };

    log_raw_frame("[RAW->TX]", header, payload, payload_len);

    return serial_write_all(fd, header, sizeof(header)) &&
           serial_write_all(fd, payload, payload_len);
}

static bool send_text(int fd, uint32_t seq, const char *from, const char *text, bool ack)
{
    ChatMsg msg = ChatMsg_init_zero;
    msg.seq = seq;
    strncpy(msg.from, from, sizeof(msg.from) - 1U);
    strncpy(msg.text, text, sizeof(msg.text) - 1U);
    msg.has_ack = true;
    msg.ack = ack;
    return send_chat(fd, &msg);
}

static void rx_reset(rx_state_t *rx)
{
    rx->expected_len = 0U;
    rx->received_len = 0U;
}

static void handle_msg(int fd, const ChatMsg *msg)
{
    printf("[MCU] seq=%u from=%s ack=%d text=%s\n",
           (unsigned)msg->seq,
           msg->from,
           (msg->has_ack && msg->ack) ? 1 : 0,
           msg->text);
    fflush(stdout);

    if (!(msg->has_ack && msg->ack)) {
        if (send_text(fd, msg->seq, "pc", "ack from pc", true)) {
            printf("[Host] TX ack from pc (seq=%u)\n", (unsigned)msg->seq);
            fflush(stdout);
        }
    }
}

static void process_payload(int fd, const uint8_t *payload, uint16_t len)
{
    ChatMsg msg = ChatMsg_init_zero;
    pb_istream_t istream = pb_istream_from_buffer(payload, len);
    if (pb_decode(&istream, ChatMsg_fields, &msg)) {
        handle_msg(fd, &msg);
    } else {
        fprintf(stderr, "decode failed\n");
    }
}

static void pump_serial(int fd, rx_state_t *rx)
{
    uint8_t buf[64];
    ssize_t rd = read(fd, buf, sizeof(buf));
    if (rd <= 0) {
        return;
    }

    for (ssize_t i = 0; i < rd; ++i) {
        uint8_t byte = buf[i];

        if (rx->received_len < PROTO_FRAME_HDR_SIZE) {
            rx->header_buf[rx->received_len++] = byte;

            if (rx->received_len == PROTO_FRAME_HDR_SIZE) {
                rx->expected_len = (uint16_t)rx->header_buf[0] |
                                   ((uint16_t)rx->header_buf[1] << 8);
                if (rx->expected_len == 0U || rx->expected_len > PROTO_MAX_PAYLOAD) {
                    rx_reset(rx);
                }
            }
            continue;
        }

        rx->payload_buf[rx->received_len - PROTO_FRAME_HDR_SIZE] = byte;
        rx->received_len++;

        if (rx->received_len == (uint16_t)(PROTO_FRAME_HDR_SIZE + rx->expected_len)) {
            log_raw_frame("[RAW->RX]", rx->header_buf, rx->payload_buf, rx->expected_len);
            process_payload(fd, rx->payload_buf, rx->expected_len);
            rx_reset(rx);
        }
    }
}

int main(int argc, char **argv)
{
    const char *dev = (argc > 1) ? argv[1] : "/dev/ttyACM0";
    int fd = serial_open(dev);
    if (fd < 0) {
        fprintf(stderr, "open serial failed: %s (%s)\n", dev, strerror(errno));
        return 1;
    }

    printf("[Host] connected to %s (115200 8N1)\n", dev);

    uint32_t seq = 1U;
    rx_state_t rx;
    rx_reset(&rx);

    if (!send_text(fd, seq++, "pc", "hello from pc", false)) {
        fprintf(stderr, "[Host] initial send failed\n");
    } else {
        printf("[Host] TX hello from pc\n");
    }

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        int maxfd = (fd > STDIN_FILENO) ? fd : STDIN_FILENO;
        int rc = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "select failed: %s\n", strerror(errno));
            break;
        }

        if (FD_ISSET(fd, &readfds)) {
            pump_serial(fd, &rx);
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char line[96];
            if (fgets(line, sizeof(line), stdin) == NULL) {
                break;
            }

            size_t n = strlen(line);
            while (n > 0U && (line[n - 1U] == '\n' || line[n - 1U] == '\r')) {
                line[n - 1U] = '\0';
                n--;
            }

            if (n == 0U) {
                continue;
            }

            if (!send_text(fd, seq++, "pc", line, false)) {
                fprintf(stderr, "[Host] send failed\n");
            } else {
                printf("[Host] TX %s\n", line);
                fflush(stdout);
            }
        }
    }

    close(fd);
    return 0;
}
