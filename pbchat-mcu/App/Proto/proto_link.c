#include "proto_link.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "pb_encode.h"
#include "pb_decode.h"

#include "chat.pb.h"
#include "uart_driver.h"

#define PROTO_FRAME_HDR_SIZE 2U
#define PROTO_MAX_PAYLOAD    ChatMsg_size

typedef struct
{
    uint16_t expected_len;
    uint16_t received_len;
    uint8_t header_buf[PROTO_FRAME_HDR_SIZE];
    uint8_t payload_buf[PROTO_MAX_PAYLOAD];
} proto_rx_state_t;

static proto_rx_state_t g_rx;
static uint32_t g_tx_seq = 1U;

static void proto_reset_rx(void)
{
    g_rx.expected_len = 0U;
    g_rx.received_len = 0U;
}

static bool proto_send_chat(const ChatMsg *msg)
{
    uint8_t payload[PROTO_MAX_PAYLOAD];
    pb_ostream_t ostream = pb_ostream_from_buffer(payload, sizeof(payload));
    if (!pb_encode(&ostream, ChatMsg_fields, msg)) {
        return false;
    }

    if (ostream.bytes_written > 0xFFFFu) {
        return false;
    }

    uint16_t payload_len = (uint16_t)ostream.bytes_written;
    uint8_t header[PROTO_FRAME_HDR_SIZE] = {
        (uint8_t)(payload_len & 0xFFu),
        (uint8_t)((payload_len >> 8) & 0xFFu)
    };

    uart_send(header, sizeof(header));
    uart_send(payload, payload_len);
    return true;
}

static void proto_send_text(const char *from, const char *text, bool ack, uint32_t seq)
{
    ChatMsg msg = ChatMsg_init_zero;

    msg.seq = seq;
    (void)strncpy(msg.from, from, sizeof(msg.from) - 1U);
    (void)strncpy(msg.text, text, sizeof(msg.text) - 1U);
    msg.has_ack = true;
    msg.ack = ack;

    (void)proto_send_chat(&msg);
}

static void proto_handle_message(const ChatMsg *msg)
{
    if (msg->has_ack && msg->ack) {
        return;
    }

    proto_send_text("mcu", "ack from stm32", true, msg->seq);
}

static void proto_try_decode_frame(const uint8_t *payload, uint16_t len)
{
    ChatMsg msg = ChatMsg_init_zero;
    pb_istream_t istream = pb_istream_from_buffer(payload, len);

    if (pb_decode(&istream, ChatMsg_fields, &msg)) {
        proto_handle_message(&msg);
    }
}

void proto_link_init(void)
{
    proto_reset_rx();
    proto_send_text("mcu", "hello from stm32", false, g_tx_seq++);
}

void proto_link_poll(void)
{
    uint8_t byte = 0U;

    while (uart_read_byte(&byte)) {
        if (g_rx.received_len < PROTO_FRAME_HDR_SIZE) {
            g_rx.header_buf[g_rx.received_len++] = byte;

            if (g_rx.received_len == PROTO_FRAME_HDR_SIZE) {
                g_rx.expected_len = (uint16_t)g_rx.header_buf[0] |
                                    ((uint16_t)g_rx.header_buf[1] << 8);
                if ((g_rx.expected_len == 0U) || (g_rx.expected_len > PROTO_MAX_PAYLOAD)) {
                    proto_reset_rx();
                }
            }
            continue;
        }

        g_rx.payload_buf[g_rx.received_len - PROTO_FRAME_HDR_SIZE] = byte;
        g_rx.received_len++;

        if (g_rx.received_len == (uint16_t)(PROTO_FRAME_HDR_SIZE + g_rx.expected_len)) {
            proto_try_decode_frame(g_rx.payload_buf, g_rx.expected_len);
            proto_reset_rx();
        }
    }
}
