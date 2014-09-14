#include <stm32f4xx.h>
#include "uart.h"

void delayMs(int ms) {
  while (ms--) {
    while ((SysTick->CTRL&SysTick_CTRL_COUNTFLAG_Msk)==0);
  }
}

int main()
{
  GPIO_InitTypeDef gpioInit = {0};

  SystemInit();
  uartInit();
  

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
  
  gpioInit.GPIO_Pin = GPIO_Pin_2;
  gpioInit.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_Init(GPIOD, &gpioInit);
  
  SysTick->LOAD = (SystemCoreClock/8)/1000; // Set systick to overflow every 1ms
  SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
  
  while(1) {
    GPIO_WriteBit(GPIOD, GPIO_Pin_2, 1);
    while(!uartIsRxReady());
    uartGetc();
    GPIO_WriteBit(GPIOD, GPIO_Pin_2, 0);
    while(!uartIsRxReady());
    uartGetc();
  }
  
  return 0;
}
