//UART driver specific to the Crazyflie 2.0 bootloader. Implements
// 'soft-hard flow control'
#include <stdbool.h>

#include <stm32f4xx.h>
#include "uart.h"

#define TXQ_LEN 64
#define RXQ_LEN 64

static char rxq[RXQ_LEN];
static int rxqTail;
static int rxqHead;

static char txq[TXQ_LEN];
static int txqTail;
static int txqHead;

static unsigned int dropped;

void USART6_IRQHandler()
{
  if (USART_GetFlagStatus(USART6, USART_FLAG_TC)) {
    USART_ClearFlag(USART6, USART_FLAG_TC);
    if (txqHead != txqTail) {
      USART_SendData(USART6, txq[txqTail]);
      txqTail = (txqTail+1)%TXQ_LEN;
    }
  }
  while (USART_GetFlagStatus(USART6, USART_FLAG_RXNE)) {
    if (((rxqHead+1)%RXQ_LEN)!=rxqTail) {
      rxq[rxqHead] = USART_ReceiveData(USART6);
      rxqHead = (rxqHead+1)%RXQ_LEN;
    } else {
      dropped++;
    }
  }
}

void uartInit()
{
  USART_InitTypeDef uartInit = {0};
  GPIO_InitTypeDef gpioInit = {0};
  
  //Uart GPIOs
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  gpioInit.GPIO_Pin = GPIO_Pin_6;
  gpioInit.GPIO_Mode = GPIO_Mode_AF;
  GPIO_Init(GPIOC, &gpioInit);
  gpioInit.GPIO_Pin = GPIO_Pin_7;
  GPIO_Init(GPIOC, &gpioInit);

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);
  uartInit.USART_BaudRate = 1000000;
  uartInit.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(USART6, &uartInit);

  // Enable interrupt
  USART_ITConfig(USART6, USART_IT_TC, ENABLE);
  USART_ITConfig(USART6, USART_IT_RXNE, ENABLE);
  NVIC_EnableIRQ(USART6_IRQn);

  //Enable flow control GPIO
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  gpioInit.GPIO_Pin = GPIO_Pin_4;
  gpioInit.GPIO_Mode = GPIO_Mode_IN;
  gpioInit.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOA, &gpioInit);
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
  return ((txqHead+1)%TXQ_LEN) != txqTail;
}

void uartPutc(char data)
{
  if (uartIsTxReady()) {
    txq[txqHead] = data;
    txqHead = ((txqHead+1)%TXQ_LEN);
  }
}
