#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <stm32f4xx.h>
#include "uart.h"
#include "syslink.h"
#include "crtp.h"
#include "loaderCommands.h"
#include "config.h"

static uint32_t tick;

static bool bootloaderProcess(CrtpPacket *packet);

void SysTick_Handler()
{
  tick++;
}

void delayMs(int ms) {
  while (ms--) {
    while ((SysTick->CTRL&SysTick_CTRL_COUNTFLAG_Msk)==0);
  }
}

int main()
{
  GPIO_InitTypeDef gpioInit = {0};
  struct syslinkPacket slPacket;
  struct crtpPacket_s packet;
  int ledGreenTime=0;
  int ledRedTime = 0;

  SystemInit();
  uartInit();
  

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
  
  gpioInit.GPIO_Pin = GPIO_Pin_2;
  gpioInit.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_Init(GPIOD, &gpioInit);
  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

  gpioInit.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_0;
  gpioInit.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_Init(GPIOC, &gpioInit);
  GPIO_WriteBit(GPIOC, GPIO_Pin_0, 1);
  GPIO_WriteBit(GPIOC, GPIO_Pin_1, 1);

  SysTick->LOAD = (SystemCoreClock/8)/1000; // Set systick to overflow every 1ms
  SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk;
  NVIC_EnableIRQ(SysTick_IRQn);
  
  // Blue LED ON by default
  GPIO_WriteBit(GPIOD, GPIO_Pin_2, 1);

  while(1) {
	if (syslinkReceive(&slPacket)) {
      if (slPacket.type == SYSLINK_RADIO_RAW) {
    	  memcpy(packet.raw, slPacket.data, slPacket.length);
    	  packet.datalen = slPacket.length-1;

    	  ledGreenTime = tick;
    	  GPIO_WriteBit(GPIOC, GPIO_Pin_1, 0);

    	  if (bootloaderProcess(&packet)) {
    	    ledRedTime = tick;
    	    GPIO_WriteBit(GPIOC, GPIO_Pin_0, 0);

    	    memcpy(slPacket.data, packet.raw, packet.datalen+1);
    	    slPacket.length = packet.datalen+1;
    	    syslinkSend(&slPacket);
    	  }
      }

      if (ledGreenTime!=0 && tick-ledGreenTime>10) {
        GPIO_WriteBit(GPIOC, GPIO_Pin_1, 1);
        ledGreenTime = 0;
      }
      if (ledRedTime!=0 && tick-ledRedTime>10) {
        GPIO_WriteBit(GPIOC, GPIO_Pin_0, 1);
        ledRedTime = 0;
      }
	}
  }
  return 0;
}

static const uint32_t sector_address[] = {
  [0]  = 0x08000000,
  [1]  = 0x08004000,
  [2]  = 0x08008000,
  [3]  = 0x0800C000,
  [4]  = 0x08010000,
  [5]  = 0x08020000,
  [6]  = 0x08040000,
  [7]  = 0x08060000,
  [8]  = 0x08080000,
  [9]  = 0x080A0000,
  [10] = 0x080C0000,
  [11] = 0x080E0000,
};

static bool bootloaderProcess(CrtpPacket *pk)
{
  static char buffer[BUFFER_PAGES*PAGE_SIZE];

  if ((pk->datalen>1) && (pk->header == 0xFF) && (pk->data[0]==0xFF))
  {
    if (pk->data[1] == CMD_GET_INFO)
    {
      GetInfoReturns_t * info = (GetInfoReturns_t *)&pk->data[2];

      info->pageSize = PAGE_SIZE;
      info->nBuffPages = BUFFER_PAGES;
      info->nFlashPages = FLASH_PAGES;
      info->flashStart = FLASH_START;
      //memcpy(info->cpuId, cpuidGetId(), CPUID_LEN);
      info->version = PROTOCOL_VERSION;

      pk->datalen = 2+sizeof(GetInfoReturns_t);

      return true;
    }/*
    else if (pk->data[1] == CMD_SET_ADDRESS)
    {
      SetAddressParameters_t * addressPk;
      addressPk = (SetAddressParameters_t *)&pk->data[2];

      radioSetAddress(addressPk->address);
    }
    else */if (pk->data[1] == CMD_LOAD_BUFFER)
    {
      int i=0;
      LoadBufferParameters_t *params = (LoadBufferParameters_t *)&pk->data[2];
      char *data = (char*) &pk->data[2+sizeof(LoadBufferParameters_t)];

      //Fill the buffer with the given data
      for(i=0; i<(pk->datalen-(2+sizeof(LoadBufferParameters_t))) && (i+(params->page*PAGE_SIZE)+params->address)<(BUFFER_PAGES*PAGE_SIZE); i++)
      {
        buffer[(i+(params->page*PAGE_SIZE)+params->address)] = data[i];
      }
    }
    else if (pk->data[1] == CMD_READ_BUFFER)
    {
      int i=0;
      ReadBufferParameters_t *params = (ReadBufferParameters_t *)&pk->data[2];
      char *data = (char*) &pk->data[2+sizeof(ReadBufferParameters_t)];

      //Return the data required
      for(i=0; i<25 && (i+(params->page*PAGE_SIZE)+params->address)<(BUFFER_PAGES*PAGE_SIZE); i++)
      {
        data[i] = buffer[(i+(params->page*PAGE_SIZE)+params->address)];
      }

      pk->datalen += i;

      return true;
    }
    else if (pk->data[1] == CMD_READ_FLASH)
    {
      int i=0;
      ReadFlashParameters_t *params = (ReadFlashParameters_t *)&pk->data[2];
      char *data = (char*) &pk->data[2+sizeof(ReadFlashParameters_t)];
      char *flash= (char*)FLASH_BASE;

      //Return the data required
      for(i=0; i<25 && (i+(params->page*PAGE_SIZE)+params->address)<(FLASH_PAGES*PAGE_SIZE); i++)
      {
        //data[i] = flash[(i+(params->page*PAGE_SIZE)+params->address)];
        //data[i] = *((char*)(FLASH_BASE+i+(params->page*PAGE_SIZE)+params->address));
        data[i] = flash[(i+(params->page*PAGE_SIZE)+params->address)];
      }

      pk->datalen += i;

      return true;
    }
    else if (pk->data[1] == CMD_WRITE_FLASH)
    {
      int i;
      int j;
      unsigned int error = 0xFF;
      int flashAddress;
      uint32_t *bufferToFlash;
      WriteFlashParameters_t *params = (WriteFlashParameters_t *)&pk->data[2];
      WriteFlashReturns_t *returns = (WriteFlashReturns_t *)&pk->data[2];

      returns->done = 1;
      returns->error = 0;
      pk->datalen = 2+sizeof(WriteFlashReturns_t);
      return true;

      //Test if it is an acceptable write request
      if ( (params->flashPage<FLASH_START) || (params->flashPage>=FLASH_PAGES) ||
          ((params->flashPage+params->nPages)>FLASH_PAGES) || (params->bufferPage>=BUFFER_PAGES)
      )
      {
        //Return a failure answer
        returns->done = 0;
        returns->error = 1;
        pk->datalen = 2+sizeof(WriteFlashReturns_t);
        return true;
      }
      // Else, if everything is OK, flash the page(s)
      else
      {
        //Erase the page(s)
        for(i=0; i<params->nPages; i++)
        {
          for (j=0; j<12; j++) {
            if ((params->nPages*PAGE_SIZE)==sector_address[j]) {
              if ( FLASH_EraseSector(j<<3, VoltageRange_3) != FLASH_COMPLETE)
              {
                error = 2;
                goto failure;
              }
            }
          }
        }

        //Write the data, long per long
        flashAddress = FLASH_BASE + (params->flashPage*PAGE_SIZE);
        bufferToFlash = (uint32_t *) (&buffer + (params->bufferPage*PAGE_SIZE));
        for (i=0; i<((params->nPages*PAGE_SIZE)/sizeof(uint32_t)); i++, flashAddress+=4)
        {
          if(FLASH_ProgramWord(flashAddress, bufferToFlash[i]) != FLASH_COMPLETE)
          {
            error = 3;
            goto failure;
          }
        }

        //Everything OK! great, send back an OK packet
        returns->done = 1;
        returns->error = 0;
        pk->datalen = 2+sizeof(WriteFlashReturns_t);
        return true;

        goto finally;

        failure:
        //If the write procedure failed, send the error packet
        //TODO: see if it is necessary or wanted to send the reason as well
        returns->done = 0;
        returns->error = error;
        pk->datalen = 2+sizeof(WriteFlashReturns_t);
        return true;

        finally:
        ;//None...
      }
    }
  }
  return false;
}
