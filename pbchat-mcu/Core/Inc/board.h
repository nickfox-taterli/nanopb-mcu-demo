#ifndef BOARD_H
#define BOARD_H

#include "stm32c0xx_ll_rcc.h"
#include "stm32c0xx_ll_bus.h"
#include "stm32c0xx_ll_system.h"
#include "stm32c0xx_ll_exti.h"
#include "stm32c0xx_ll_cortex.h"
#include "stm32c0xx_ll_utils.h"
#include "stm32c0xx_ll_pwr.h"
#include "stm32c0xx_ll_dma.h"
#include "stm32c0xx_ll_usart.h"
#include "stm32c0xx_ll_gpio.h"

/* ---- Clock ---- */
#define BOARD_HSI_FREQ         48000000U
#define BOARD_HSE_FREQ         8000000U
#define BOARD_LSE_FREQ         32768U
#define BOARD_LSI_FREQ         32000U

/* ---- USART2 ---- */
#define BOARD_USART_INSTANCE   USART2
#define BOARD_USART_BAUDRATE   115200U

/* ---- USART2 GPIO (PA2=TX, PA3=RX, AF1) ---- */
#define BOARD_USART_TX_PORT    GPIOA
#define BOARD_USART_TX_PIN     LL_GPIO_PIN_2
#define BOARD_USART_TX_AF      LL_GPIO_AF_1
#define BOARD_USART_RX_PORT    GPIOA
#define BOARD_USART_RX_PIN     LL_GPIO_PIN_3
#define BOARD_USART_RX_AF      LL_GPIO_AF_1

void LowLevel_Init(void);

#endif /* BOARD_H */
