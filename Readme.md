# Crazyflie 2 STM Bootloader [![CI](https://github.com/bitcraze/crazyflie2-stm-bootloader/workflows/CI/badge.svg)](https://github.com/bitcraze/crazyflie2-stm-bootloader/actions?query=workflow%3ACI)

Bootloader for the Crazyflie 2

## Architecture
The Bootloader sits at the beginning of the flash memory 1) and handles the memory in pages:
```
+------------+ <- flashPages
 |            |
 |            |
 |            |
 |            |
 |            |
 |            |
 |            |
 |            |
 |            |
 |            |
 |  Firmware  |
 |            |
 |            |
 +------------+ <- flashStart
 | Bootloader |
 +------------+ <- Page 0
```

 The size of the pages, the values flashStart and flashPages are communicated by the protocol. For simplicity of the bootloader implementation this architecture is exposed to the protocol
```
    +---------------+                      +--------------+
   | Buffer page n |                      | Flash Page m |
   +---------------+                      +--------------+
   \               \                      \              \
   \               \                      \              \
   +---------------+      +-------+       +--------------+
   | Buffer page 0 |----->| Flash |------>| Flash Page 0 |
   +---------------+      +-------+       +--------------+
           ^                  ^                  |        
           |                  |                  |
           v                  |                  v
   +------------------------------------------------------+
   |                   Bootloader control                 |
   +------------------------------------------------------+
```

The procedure to flash is then to send the data to the buffer, and then order the bootloader to flash the buffer(s) in flash. All parameters like page size, number of flash page, number of buffer pages, are accessible via the protocol so that it is possible to change these physical values without changing the bootloader client.

## Protocol

All packets sent to the bootloader starts with “0xFF Target_number”, this kind of packets are called 'NULL Parcket' by CRTP and are ignored by the Crazyflie firmware. The target number has been introduced with Crazyflie 2 as it contains 2 bootloader. The mapping is as follow:

| Board       | Target_number  | MCU | Protocol version |
| -----       | -------------  | --- | ---------------- |
| Crazyflie 1 | 0xFF          | STM32F103 | 0x00 |
| Crazyflie 2 | 0xFF          | STM32F405 | 0x10 |
|    :::      | 0xFE          | nRF51822  | 0x10 |

The high nibble of the version aims at describing the board if board-specific change is required to the protocol (ie. GET_MAPPING that is specific to stm32f405).

All packets have the following format:

<ditaa noedgesep>
 +------+---------------+---------+======+
 | 0xFF | Target_number | Command | Data |
 +------+---------------+---------+======+
    1           1            1      0-28    Bytes
</ditaa>

In the rest of this page commands and data are described.

### Commands summary

| Command  | Name  | Note |
| -------  | ----  | ---- |
| 0x10  | GET_INFO  |   |
| 0x11  | SET_ADDRESS  | Only implemented on Crazyflie version 0x00 |
| 0x12  | GET_MAPPING  | Only implemented in version 0x10 target 0xFF  |
| 0x14  | LOAD_BUFFER  |   |
| 0x15  | READ_BUFFER  |   |
| 0x18  | WRITE_FLASH  |   |
| 0x19  | FLASH_STATUS  |   |
| 0x1C  | READ_FLASH  |   |
| 0xFF  | RESET_INIT  | Only implemented in version 0x10 target 0xFE |
| 0xF0  | RESET | Only implemented in version 0x10 target 0xFE |
| 0x01  | ALLOFF | Only implemented in version 0x10 target 0xFE |
| 0x02  | SYSOFF | Only implemented in version 0x10 target 0xFE |
| 0x03  | SYSON | Only implemented in version 0x10 target 0xFE |
| 0x04  | GETVBAT | Only implemented in version 0x10 target 0xFE |

#### GET_INFO

| Byte | Request fields | Content  |
| ---- | -------------- | -------  |
|  0   | GET_INFO       | 0x10 |

| Byte | Answer fields  | Content  |
| ---- | -------------- | -------  |
|  0   | GET_INFO       | 0x10 |
| 1-2  | pageSize       | Size in byte of flash and buffer page |
| 3-4  | nBuffPage      | Number of ram buffer page available |
| 5-6  | nFlashPage     | Total number of flash page |
| 7-8  | flashStart     | Start flash page of the firmware |
| 9-21 | cpuId          | Legacy 12Bytes CPUID, shall be ignored |
|  22  | version        | Version of the protocol |


This exchange requests the bootloader info. The content of this packet contains all information required to program the flash.

#### SET_ADDRESS

| Byte | Request fields | Content |
| ---- | -------------- | -------  |
|  0   | SET_ADDRESS    | 0x11 |
|  1-6  | address        | 5 Bytes radio (ESB) address |

Sets the radio address. This allows to make sure no other computer will interfere with the flash process. It is not mandatory.

#### GET_MAPPING

| Byte | Request fields | Content  |
| ---- | -------------- | -------  |
|  0   | GET_MAPPING       | 0x12 |

| Byte | Answer fields  | Content  |
| ---- | -------------- | -------  |
|  0   | GET_MAPPING       | 0x12 |
|  1-..  | mapping | Erase page mapping of the flash, up to 27 bytes |

For all supported MCU except the STM32F405 when flashing one page the page is erased and then written with the new data.

The STM32F405 though has a very large flash memory (1M) and fairly large erase sector that have a different size. On this chip the bootloader is setup with 1024Bytes pages and when a page happens to be the first of a sector the full sector is erased. This command allows to request the size of the sectors.

The mapping field is encoded as a sequence of [Number of sector, Size of sector in page]. For example the STM32F405 has a mapping of [4, 16, 1, 64, 7, 128] because it contains 4 sector of 16KB, 1 of 64KB and 7 of 128KB. If this command where implemented in the STM32F103 the mapping would be [128, 1].

#### LOAD_BUFFER

| Byte | Request fields | Content  |
| ---- | -------------- | -------  |
|  0   | LOAD_BUFFER       | 0x14 |
|  1-2  | page | Buffer page to load into |
|  3-4  | Address | Address in the buffer page to load from |
|  5-31 | data | Data to load |

#### READ_BUFFER

| Byte | Request fields | Content  |
| ---- | -------------- | -------  |
|  0   | READ_BUFFER       | 0x15 |
|  1-2  | page | Buffer page to read |
|  3-4  | Address | Address in the buffer page to read from |

| Byte | Answer fields | Content  |
| ---- | ------------- | -------  |
|  0   | READ_BUFFER       | 0x15 |
|  1-2  | page | Buffer page read |
|  3-4  | Address | Address in the buffer page read from |
|  5-31 | data | Data read |

#### WRITE_FLASH

| Byte | Request fields | Content  |
| ---- | -------------- | -------  |
|  0   | WRITE_BUFFER | 0x18 |
|  1-2  | bufferPage | Buffer page source |
|  3-4  | flashPage | Flash page destination |
|  5-6  | nPages | Number of page to program |

| Byte | Answer fields | Content  |
| ---- | ------------- | -------  |
|  0   | WRITE_FLASH | 0x18 |
|  1   | done  | At 0 if the operation failed, not 0 otherwise |
|  2   | Error | 0 if no error, otherwise contains error code |

The error code can be:

| Code  | Meaning  |
| ----  | -------  |
| 0  | No error |
| 1  | Addresses are outside of authorized boundaries |
| 2  | Flash erase failed  |
| 3  | Flash programming failed  |

#### FLASH_STATUS

| Byte | Request fields | Content  |
| ---- | -------------- | -------  |
|  0   | FLASH_STATUS | 0x19 |

| Byte | Answer fields | Content  |
| ---- | -------------- | -------  |
|  0   | FLASH_STATUS | 0x19 |
|  1   | done  | At 0 if the operation failed, not 0 otherwise |
|  2   | Error | 0 if no error, otherwise contains error code |

This message aims at checking the latest flash operation in case where the WRITE_FLASH answer would be lost. The error code follows the same format as for WRITE_FLASH.

#### READ_FLASH

| Byte | Request fields | Content  |
| ---- | -------------- | -------  |
|  0   | READ_FLASH       | 0x1C |
|  1-2  | page | Flash page to read |
|  3-4  | Address | Address in the flash page to read from |

| Byte | Answer fields | Content  |
| ---- | -------------- | -------  |
|  0   | READ_FLASH       | 0x1C |
|  1-2  | page | Flash page read |
|  3-4  | Address | Address in the flash page read from |
|  5-31 | data | Data read |

####  RESET_INIT

Prepare to reset (no additional data fields). The result will be the original request.

####  RESET

Reset (no additional data fields). No result will be sent.

####  ALLOFF

Turn everything off as if the power button would have been pressed (i.e. STM32 and radio). The CF won't be able to wake-up unless the power button is pressed. No result will be sent.

####  SYSOFF

Turn the STM32 off, but keep the NRF51 with radio awake. The CF can be woken up by: sending reset, pressing the power button, or sendind SYSON. No result will be sent.

####  SYSON

Turn the STM32 on. No result will be sent.

####  GETVBAT

| Byte | Answer fields | Content  |
| ---- | ------------- | -------  |
|  0-3   | vbat | floating point value containing the current battery voltage in volts |

Check battery status (works even if the STM32 is turned off). 

## Contribute
Go to the [contribute page](https://www.bitcraze.io/contribute/) on our website to learn more.

### Test code for contribution
Run the automated build locally to test your code

	./tools/build/build

