# CrazyFlie's Makefile
# Copyright (c) 2011,2012 Bitcraze AB
# This Makefile compiles all the objet file to ./bin/ and the resulting firmware
# image in ./cflie.elf and ./cflie.bin

#Put your personal build config in config.mk and DO NOT COMMIT IT!
-include config.mk

######### JTAG and environment configuration ##########
OPENOCD           ?= openocd
OPENOCD_INTERFACE ?= interface/stlink-v2.cfg
OPENOCD_TARGET    ?= target/stm32f4x_stlink.cfg
OPENOCD_CMDS      ?=
CROSS_COMPILE     ?= arm-none-eabi-
PYTHON2           ?= python
CLOAD             ?= 0
DEBUG             ?= 1
CLOAD_SCRIPT      ?= ../crazyflie-pc-client/bin/cfloader

# Now needed for SYSLINK
CFLAGS += -DUSE_SYSLINK_CRTP     # Set CRTP link to syslink
CFLAGS += -DENABLE_UART          # To enable the uart

## Flag that can be added to config.mk
# CFLAGS += -DUSE_ESKYLINK         # Set CRTP link to E-SKY receiver
# CFLAGS += -DDEBUG_PRINT_ON_UART  # Redirect the console output to the UART

REV               ?= E

#OpenOCD conf
RTOS_DEBUG        ?= 0

############### Location configuration ################

STLIB = lib/

################ Build configuration ##################
# St Lib
VPATH += $(STLIB)/CMSIS/STM32F4xx/Source/
VPATH += $(STLIB)/STM32_CPAL_Driver/src
VPATH += $(STLIB)/STM32_CPAL_Driver/devices/stm32f4xx
VPATH += $(STLIB)/STM32F4xx_StdPeriph_Driver/src
CRT0 = startup_stm32f40xx.o system_stm32f4xx.o

# Should maybe be in separate file?
#-include scripts/st_f405_obj.mk
ST_OBJ += stm32f4xx_rcc.o stm32f4xx_gpio.o stm32f4xx_usart.o stm32f4xx_misc.o stm32f4xx_flash.o
# ST_OBJ += cpal_hal.o cpal_i2c.o cpal_usercallback_template.o cpal_i2c_hal_stm32f4xx.o

# Crazyflie
VPATH += src


############### Source files configuration ################

PROJ_OBJ = main.o uart.o syslink.o bootpin.o


OBJ = $(CRT0) $(ST_OBJ) $(PROJ_OBJ)

ifdef P
  C_PROFILE = -D P_$(P)
endif

############### Compilation configuration ################
AS = $(CROSS_COMPILE)as
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)gcc
SIZE = $(CROSS_COMPILE)size
OBJCOPY = $(CROSS_COMPILE)objcopy


INCLUDES = -Iinclude -I$(PORT) -I.
INCLUDES+= -Iconfig -Ihal/interface -Imodules/interface
INCLUDES+= -Iutils/interface -Idrivers/interface -Iplatform
INCLUDES+= -I$(STLIB)/CMSIS/Include

INCLUDES+= -I$(STLIB)/STM32F4xx_StdPeriph_Driver/inc
INCLUDES+= -I$(STLIB)/STM32_CPAL_Driver/inc
INCLUDES+= -I$(STLIB)/STM32_CPAL_Driver/devices/stm32f4xx
INCLUDES+= -I$(STLIB)/CMSIS/STM32F4xx/Include 

PROCESSOR = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16

#Flags required by the ST library
STFLAGS = -DSTM32F4XX -DSTM32F40_41xxx -DHSE_VALUE=8000000 -DUSE_STDPERIPH_DRIVER


ifeq ($(DEBUG), 1)
  CFLAGS += -O0 -g3
else
  CFLAGS += -Os -g3
endif

ifeq ($(LTO), 1)
  CFLAGS += -flto
endif

CFLAGS += -DBOARD_REV_$(REV)

CFLAGS += $(PROCESSOR) $(INCLUDES) $(STFLAGS) -Wall -fno-strict-aliasing $(C_PROFILE)
# Compiler flags to generate dependency files:
CFLAGS += -MD -MP -MF $(BIN)/dep/$(@).d -MQ $(@)
#Permits to remove un-used functions and global variables from output file
CFLAGS += -ffunction-sections -fdata-sections

ASFLAGS = $(PROCESSOR) $(INCLUDES)
LDFLAGS = $(PROCESSOR) -Wl,-Map=$(PROG).map,--cref,--gc-sections

ifeq ($(CLOAD), 1)
  LDFLAGS += -T scripts/linker/STM32F103_32K_20K_FLASH_CLOAD.ld
else
  LDFLAGS += -T scripts/linker/STM32F103_32K_20K_FLASH.ld
endif

ifeq ($(LTO), 1)
  LDFLAGS += -Os -flto -fuse-linker-plugin
endif

#Program name
PROG = cf2loader
#Where to compile the .o
BIN = bin
VPATH += $(BIN)

#Dependency files to include
DEPS := $(foreach o,$(OBJ),$(BIN)/dep/$(o).d)

##################### Misc. ################################
ifeq ($(SHELL),/bin/sh)
  COL_RED=\033[1;31m
  COL_GREEN=\033[1;32m
  COL_RESET=\033[m
endif

#################### Targets ###############################


all: build
build: clean_version compile print_version size
compile: clean_version $(PROG).hex $(PROG).bin

clean_version:
ifeq ($(SHELL),/bin/sh)
	@echo "  CLEAN_VERSION"
	@rm -f version.c
endif

print_version: compile
ifeq ($(SHELL),/bin/sh)
	@./scripts/print_revision.sh
endif
ifeq ($(CLOAD), 1)
	@echo "CrazyLoader build!"
endif

size: compile
	@$(SIZE) -B $(PROG).elf

#Radio bootloader
cload:
ifeq ($(CLOAD), 1)
	$(CLOAD_SCRIPT) flash cflie.bin
else
	@echo "Only cload build can be bootloaded. Launch build and cload with CLOAD=1"
endif

#Flash the stm.
flash:
	$(OPENOCD) -d2 -f $(OPENOCD_INTERFACE) $(OPENOCD_CMDS) -f $(OPENOCD_TARGET) -c init -c targets -c "reset halt" \
                 -c "flash write_image erase $(PROG).elf" -c "verify_image $(PROG).elf" -c "reset run" -c shutdown

#STM utility targets
halt:
	$(OPENOCD) -d0 -f $(OPENOCD_INTERFACE) $(OPENOCD_CMDS) -f $(OPENOCD_TARGET) -c init -c targets -c "halt" -c shutdown

reset:
	$(OPENOCD) -d0 -f $(OPENOCD_INTERFACE) $(OPENOCD_CMDS) -f $(OPENOCD_TARGET) -c init -c targets -c "reset" -c shutdown

openocd:
	$(OPENOCD) -d2 -f $(OPENOCD_INTERFACE) $(OPENOCD_CMDS) -f $(OPENOCD_TARGET) -c init -c targets

#Print preprocessor #defines
prep:
	@$(CC) -dD

include scripts/targets.mk

#include dependencies
-include $(DEPS)
