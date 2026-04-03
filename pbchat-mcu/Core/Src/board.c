#include "board.h"

static void SystemClock_Config(void);
static void USART2_Init(void);

void LowLevel_Init(void)
{
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

  SystemClock_Config();
  USART2_Init();
}

static void SystemClock_Config(void)
{
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOC);
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOF);
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);

  LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);

  LL_RCC_HSI_Enable();
  while (LL_RCC_HSI_IsReady() != 1)
  {
  }

  LL_RCC_HSI_SetCalibTrimming(64);
  LL_RCC_HSI_SetCalibTrimming(64);
  LL_RCC_SetHSIDiv(LL_RCC_HSI_DIV_1);
  LL_RCC_SetAHBPrescaler(LL_RCC_HCLK_DIV_1);

  LL_RCC_SetSYSDivider(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);
  while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI)
  {
  }

  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_Init1msTick(BOARD_HSI_FREQ);
  LL_SetSystemCoreClock(BOARD_HSI_FREQ);
}

static void USART2_Init(void)
{
  LL_USART_InitTypeDef USART_InitStruct = {0};
  LL_GPIO_InitTypeDef  GPIO_InitStruct  = {0};

  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);

  /* PA2 -> USART2_TX */
  GPIO_InitStruct.Pin        = BOARD_USART_TX_PIN;
  GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate  = BOARD_USART_TX_AF;
  LL_GPIO_Init(BOARD_USART_TX_PORT, &GPIO_InitStruct);

  /* PA3 -> USART2_RX */
  GPIO_InitStruct.Pin        = BOARD_USART_RX_PIN;
  GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate  = BOARD_USART_RX_AF;
  LL_GPIO_Init(BOARD_USART_RX_PORT, &GPIO_InitStruct);

  USART_InitStruct.PrescalerValue     = LL_USART_PRESCALER_DIV1;
  USART_InitStruct.BaudRate           = BOARD_USART_BAUDRATE;
  USART_InitStruct.DataWidth          = LL_USART_DATAWIDTH_8B;
  USART_InitStruct.StopBits           = LL_USART_STOPBITS_1;
  USART_InitStruct.Parity             = LL_USART_PARITY_NONE;
  USART_InitStruct.TransferDirection  = LL_USART_DIRECTION_TX_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
  USART_InitStruct.OverSampling       = LL_USART_OVERSAMPLING_16;
  LL_USART_Init(BOARD_USART_INSTANCE, &USART_InitStruct);

  LL_USART_ConfigAsyncMode(BOARD_USART_INSTANCE);
  LL_USART_Enable(BOARD_USART_INSTANCE);

  while ((!(LL_USART_IsActiveFlag_TEACK(BOARD_USART_INSTANCE)))
      || (!(LL_USART_IsActiveFlag_REACK(BOARD_USART_INSTANCE))))
  {
  }

  /* Enable RXNE interrupt + NVIC */
  LL_USART_EnableIT_RXNE(BOARD_USART_INSTANCE);
  NVIC_SetPriority(USART2_IRQn, 0);
  NVIC_EnableIRQ(USART2_IRQn);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
