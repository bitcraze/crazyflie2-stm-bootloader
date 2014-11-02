//UART driver specific to the Crazyflie 2.0 bootloader. Implements
// 'soft-hard flow control'
#include <stdbool.h>

#include <stm32f4xx.h>
#include "uart.h"

#define POLLING_TX

#define TXQ_LEN 64
#define RXQ_LEN 64

static char rxq[RXQ_LEN];
static int rxqTail;
static int rxqHead;

#ifndef POLLING_TX
static char txq[TXQ_LEN];
static int txqTail;
static int txqHead;
#endif

static unsigned int dropped;
static volatile int devnull;

void USART6_IRQHandler()
{
#ifndef POLLING_TX
  if (USART_GetFlagStatus(USART6, USART_FLAG_TC)) {
    USART_ClearFlag(USART6, USART_FLAG_TC);
    if (txqHead != txqTail) {
      USART_SendData(USART6, txq[txqTail]);
      txqTail = (txqTail+1)%TXQ_LEN;
    }
  }
#endif
  if (USART_GetFlagStatus(USART6, USART_FLAG_RXNE)) {
    if (((rxqHead+1)%RXQ_LEN)!=rxqTail) {
      rxq[rxqHead] = USART_ReceiveData(USART6);
      rxqHead = (rxqHead+1)%RXQ_LEN;
    } else {
      devnull = USART_ReceiveData(USART6); //Drop data
      dropped++;
    }
  }
}

void uartInit()
{
  USART_InitTypeDef usartInit = {0};
  GPIO_InitTypeDef gpioInit = {0};
  NVIC_InitTypeDef nvicInit = {0};
  
  //Enable clocks
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

  //Uart GPIOs
  gpioInit.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
  gpioInit.GPIO_Mode = GPIO_Mode_AF;
  gpioInit.GPIO_OType = GPIO_OType_PP;
  gpioInit.GPIO_Speed = GPIO_Speed_25MHz;
  gpioInit.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOC, &gpioInit);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_USART6);
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_USART6);

  usartInit.USART_BaudRate = 1000000;
  usartInit.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  usartInit.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  usartInit.USART_Parity = USART_Parity_No;
  usartInit.USART_WordLength = USART_WordLength_8b;
  usartInit.USART_StopBits = USART_StopBits_1;
  USART_Init(USART6, &usartInit);

  // Enable interrupt
#ifndef POLLING_TX
  USART_ITConfig(USART6, USART_IT_TC, ENABLE);
#endif
  USART_ITConfig(USART6, USART_IT_RXNE, ENABLE);
  nvicInit.NVIC_IRQChannel = USART6_IRQn;
  nvicInit.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&nvicInit);
  //NVIC_EnableIRQ(USART6_IRQn);

  //Enable flow control GPIO
  gpioInit.GPIO_Pin = GPIO_Pin_4;
  gpioInit.GPIO_Mode = GPIO_Mode_IN;
  gpioInit.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOA, &gpioInit);

  USART_Cmd(USART6, ENABLE);
}

bool uartIsRxReady()
{
  return rxqHead != rxqTail;
}

char uartGetc()
{
  char data = 0;
  if (uartIsRxReady()) {
    data = rxq[rxqTail];
    rxqTail = (rxqTail+1)%RXQ_LEN;
  }
  return data;
}

bool uartIsTxReady()
{
#ifdef POLLING_TX
  return true;
#else
  return ((txqHead+1)%TXQ_LEN) != txqTail;
#endif
}

void uartPutc(char data)
{
#ifdef POLLING_TX
  while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4) == Bit_SET);
  USART_SendData(USART6, data);
  while(USART_GetFlagStatus(USART6, USART_FLAG_TC) == RESET);
#else
  if (uartIsTxReady()) {
    txq[txqHead] = data;
    txqHead = ((txqHead+1)%TXQ_LEN);

    if (USART_GetFlagStatus(USART6, USART_FLAG_TXE)) {
      USART_SendData(USART6, txq[txqTail]);
      txqTail = (txqTail+1)%TXQ_LEN;
    }
  } else {
	  dropped++;
  }
#endif
}

void uartPuts(char *string)
{
  while (*string) {
    uartPutc(*string++);
  }
}
