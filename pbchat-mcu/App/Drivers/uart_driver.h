#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>
#include <stddef.h>

/* Ring buffer size (power of 2 recommended for mask-based wrap) */
#define UART_RX_BUF_SIZE  64U

/* Callback type: called from ISR context when a new byte is received */
typedef void (*uart_rx_byte_cb)(uint8_t byte);

/* ---- Public API ---- */

/**
 * Send `size` bytes from `buf` over UART (blocking, polled).
 * Safe to call after USART hardware is initialised and enabled.
 */
void uart_send(const uint8_t *buf, size_t size);

/**
 * Read one byte from the receive ring buffer.
 * Returns 1 on success (byte written to *out), 0 if buffer empty.
 */
int uart_read_byte(uint8_t *out);

/**
 * Register a callback that fires (in ISR context) for every received byte.
 * Pass NULL to disable.
 */
void uart_set_rx_callback(uart_rx_byte_cb cb);

/**
 * Query how many bytes are available in the receive ring buffer.
 */
size_t uart_rx_available(void);

#endif /* UART_DRIVER_H */
