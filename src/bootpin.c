/* Pin scheme used by the NRF to direct bootloader boot:
 * State           | NRF_TX | NRF_FC |
 * ----------------+--------+--------+
 * Boot Firmware   |  HIGH  |  LOW   |
 * Boot Bootloader |  HIGH  |  HIGH  |
 * Error, boot DFU |  HZ    |  HZ    |
 *
 * NRF_TX and NRF_FC are configured with pull-down resistors
 * to detect the HZ state.
 */

#include "bootpin.h"

#include <stm32f4xx.h>

void bootpinInit(void)
{
  GPIO_InitTypeDef gpioInit;

  RCC_AHB1PeriphClockCmd(RCC_AHB1ENR_GPIOAEN, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1ENR_GPIOCEN, ENABLE);

  gpioInit.GPIO_Mode = GPIO_Mode_IN;
  gpioInit.GPIO_PuPd = GPIO_PuPd_DOWN;
  gpioInit.GPIO_Speed = GPIO_Speed_2MHz;
  gpioInit.GPIO_Pin = GPIO_Pin_4;
  GPIO_Init(GPIOA, &gpioInit);
  gpioInit.GPIO_Pin = GPIO_Pin_7;
  GPIO_Init(GPIOC, &gpioInit);
}

void bootpinDeinit(void)
{
  GPIO_DeInit(GPIOA);
  GPIO_DeInit(GPIOC);

  RCC_AHB1PeriphClockCmd(RCC_AHB1ENR_GPIOAEN, DISABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1ENR_GPIOCEN, DISABLE);
}

bool bootpinStartFirmware(void)
{
  return GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4) == Bit_RESET &&
         GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_7) == Bit_SET;
}

bool bootpinStartBootloader(void)
{
  return GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4) == Bit_SET &&
         GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_7) == Bit_SET;
}

bool bootpinNrfReset(void)
{
  return GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_4) == Bit_RESET &&
         GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_7) == Bit_RESET;
}
