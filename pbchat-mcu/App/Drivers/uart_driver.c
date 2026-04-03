#include "uart_driver.h"
#include "board.h"

/* ---- Ring buffer (lock-free single-producer / single-consumer) ---- */
static volatile uint8_t  rx_buf[UART_RX_BUF_SIZE];
static volatile uint32_t rx_head;   /* written by ISR  */
static volatile uint32_t rx_tail;   /* read  by application */

static uart_rx_byte_cb rx_callback;

/* ------------------------------------------------------------------ */
/*  TX: blocking polled transmit                                      */
/* ------------------------------------------------------------------ */
void uart_send(const uint8_t *buf, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        /* Wait until TXDR (TX data register) is empty */
        while (!LL_USART_IsActiveFlag_TXE(BOARD_USART_INSTANCE)) {
            /* busy-wait */
        }
        LL_USART_TransmitData8(BOARD_USART_INSTANCE, buf[i]);
    }
    /* Optionally wait for last byte to fully shift out */
    while (!LL_USART_IsActiveFlag_TC(BOARD_USART_INSTANCE)) {
    }
}

/* ------------------------------------------------------------------ */
/*  RX: ring-buffer read (application context)                        */
/* ------------------------------------------------------------------ */
int uart_read_byte(uint8_t *out)
{
    if (rx_head == rx_tail) {
        return 0;   /* empty */
    }
    *out = (uint8_t)rx_buf[rx_tail];
    rx_tail = (rx_tail + 1U) & (UART_RX_BUF_SIZE - 1U);
    return 1;
}

size_t uart_rx_available(void)
{
    uint32_t h = rx_head;
    uint32_t t = rx_tail;
    return (h - t) & (UART_RX_BUF_SIZE - 1U);
}

/* ------------------------------------------------------------------ */
/*  Callback registration                                             */
/* ------------------------------------------------------------------ */
void uart_set_rx_callback(uart_rx_byte_cb cb)
{
    rx_callback = cb;
}

/* ------------------------------------------------------------------ */
/*  ISR handler                      */
/* ------------------------------------------------------------------ */
void USART2_IRQHandler(void)
{
    if (LL_USART_IsActiveFlag_RXNE(BOARD_USART_INSTANCE)) {
        uint8_t byte = LL_USART_ReceiveData8(BOARD_USART_INSTANCE);

        /* Push into ring buffer (drop oldest if full) */
        uint32_t next = (rx_head + 1U) & (UART_RX_BUF_SIZE - 1U);
        if (next != rx_tail) {      /* not full */
            rx_buf[rx_head] = byte;
            rx_head = next;
        }

        if (rx_callback) {
            rx_callback(byte);
        }
    }

    /* Clear overrun flag if it occurs */
    if (LL_USART_IsActiveFlag_ORE(BOARD_USART_INSTANCE)) {
        LL_USART_ClearFlag_ORE(BOARD_USART_INSTANCE);
    }
}
