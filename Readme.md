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
| Crazyflie 2 | 0xFF          | STM32F405 | 0x10, 0x11 |
|    :::      | 0xFE          | nRF51822  | 0x10, 0x11 |

The high nibble of the version aims at describing the board if board-specific change is required to the protocol (ie. GET_MAPPING that is specific to stm32f405).

Version 0x11 adds PAGE_CRC and RANGE_CRC on both targets, and SET_ADDRESS, SET_BROADCAST_ADDRESS and SET_CHANNEL on target 0xFE. Together they allow flashing several Crazyflies at once, see [Flashing several Crazyflies at once](#flashing-several-crazyflies-at-once).

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
| 0x11  | SET_ADDRESS  | Only implemented on Crazyflie version 0x00, and from version 0x11 on target 0xFE |
| 0x12  | GET_MAPPING  | Only implemented in version 0x10 target 0xFF  |
| 0x14  | LOAD_BUFFER  |   |
| 0x15  | READ_BUFFER  |   |
| 0x18  | WRITE_FLASH  |   |
| 0x19  | FLASH_STATUS  |   |
| 0x1C  | READ_FLASH  |   |
| 0x20  | PAGE_CRC  | Only implemented from version 0x11 |
| 0x21  | SET_BROADCAST_ADDRESS  | Only implemented from version 0x11 on target 0xFE |
| 0x22  | RANGE_CRC  | Only implemented from version 0x11 |
| 0x23  | SET_CHANNEL  | Only implemented from version 0x11 on target 0xFE |
| 0xFF  | RESET_INIT  | Only implemented in version 0x10 target 0xFE |
| 0xF0  | RESET | Only implemented in version 0x10 target 0xFE |
| 0x01  | ALLOFF | Only implemented in version 0x10 target 0xFE |
| 0x02  | SYSOFF | Only implemented in version 0x10 target 0xFE |
| 0x03  | SYSON | Only implemented in version 0x10 target 0xFE |
| 0x04  | GETVBAT | Only implemented in version 0x10 target 0xFE |
| 0x05  | LED_ON | Only implemented in version 0x10 target 0xFE |
| 0x06  | LED_OFF | Only implemented in version 0x10 target 0xFE |
| 0x07  | STM32_DFU | Only implemented by the nRF51 firmware, target 0xFE |

The commands from 0x01 to 0x07 are handled by the nRF51 firmware, so they are used while the Crazyflie is running its firmware rather than in the bootloader.

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
| 9-20 | cpuId          | Legacy 12Bytes CPUID, shall be ignored |
|  21  | version        | Version of the protocol |
| 22-23 | versionMajor  | Target 0xFE only: major version of the bootloader. Bit 15 is set if it was built from a modified source tree |
|  24  | versionMinor   | Target 0xFE only: minor version of the bootloader |
|  25  | versionPatch   | Target 0xFE only: patch version of the bootloader |


This exchange requests the bootloader info. The content of this packet contains all information required to program the flash.

The bootloader version fields are only sent by the nRF51 bootloaders that support them. A bootloader built from the git tag `2024.10` reports major 2024, minor 10 and patch 0.

#### SET_ADDRESS

| Byte | Request fields | Content |
| ---- | -------------- | -------  |
|  0   | SET_ADDRESS    | 0x11 |
|  1-5  | address        | 5 Bytes radio (ESB) address |

Sets the radio address. This allows to make sure no other computer will interfere with the flash process. It is not mandatory.

On target 0xFE the address is given in the order it is written in a radio URI, for example `E7 E7 E7 E7 01` for `radio://0/80/2M/E7E7E7E701`. There is no answer, the bootloader switches to the new address at the start of its next radio timeslot, a few milliseconds later.

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

#### PAGE_CRC

| Byte | Request fields | Content  |
| ---- | -------------- | -------  |
|  0   | PAGE_CRC       | 0x20 |
|  1-2  | page | Flash page to checksum |

| Byte | Answer fields | Content  |
| ---- | -------------- | -------  |
|  0   | PAGE_CRC       | 0x20 |
|  1-2  | page | Flash page |
|  3-6  | crc32 | CRC32 of the page, 0 on error |
|  7  | error | 0 if no error, 1 if the page is outside of the flash |

Returns the CRC32 of one flash page (pageSize bytes, see GET_INFO), so a page can be verified without reading it back.

The CRC32 is the standard one used by zlib and Ethernet: reflected polynomial 0xEDB88320, initial value 0xFFFFFFFF and final XOR 0xFFFFFFFF.

#### SET_BROADCAST_ADDRESS

| Byte | Request fields | Content |
| ---- | -------------- | -------  |
|  0   | SET_BROADCAST_ADDRESS    | 0x21 |
|  1-5  | address        | 5 Bytes radio (ESB) broadcast address |

| Byte | Answer fields | Content  |
| ---- | -------------- | -------  |
|  0   | SET_BROADCAST_ADDRESS       | 0x21 |

Target 0xFE only. Makes the nRF51 bootloader also listen on a broadcast address, given in the same order as for SET_ADDRESS. Packets received on the broadcast address are processed like any other bootloader packet, for both targets, but they are not acknowledged and never answered. Bluetooth advertising is stopped, since it interrupts the radio and broadcast packets sent meanwhile would be lost. The broadcast address stays active until the Crazyflie is restarted.

#### RANGE_CRC

| Byte | Request fields | Content  |
| ---- | -------------- | -------  |
|  0   | RANGE_CRC       | 0x22 |
|  1-4  | address | Start of the range, counted from the start of the flash |
|  5-8  | length | Length of the range in bytes |

| Byte | Answer fields | Content  |
| ---- | -------------- | -------  |
|  0   | RANGE_CRC       | 0x22 |
|  1-4  | address | Start of the range |
|  5-8  | length | Length of the range |
|  9-12  | crc32 | CRC32 of the range, 0 on error |
|  13  | error | 0 if no error, 1 if the range is outside of the flash |

Returns the CRC32 of any byte range of the flash, so a whole firmware image can be verified with one request. The address is counted from the start of the flash, so address 0 is 0x08000000 on the STM32F405 and 0x00000000 on the nRF51. All fields are little endian and the CRC32 is the same as for PAGE_CRC. Checksumming the whole flash takes about 0.3 s on the STM32F405 and 0.4 s on the nRF51.

#### SET_CHANNEL

| Byte | Request fields | Content |
| ---- | -------------- | -------  |
|  0   | SET_CHANNEL    | 0x23 |
|  1   | channel        | Radio channel, 0 to 125 |

Target 0xFE only. Moves the nRF51 bootloader to another radio channel. The command is acknowledged on the current channel and not answered, the bootloader changes channel at the start of its next radio timeslot, a few milliseconds later. Check that it answers on the new channel, for example with GET_INFO. The bootloader goes back to channel 0 when it is restarted.

This lets several radios, each on its own channel, talk to different Crazyflies at the same time.

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

####  LED_ON

Turn the blue LED on. No result will be sent.

####  LED_OFF

Turn the blue LED off. No result will be sent.

####  STM32_DFU

Restart the STM32 into its ROM bootloader, so it can be flashed over USB DFU. This does the same as holding the power button from off: the nRF51 power-cycles the STM32 with BOOT0 high, and the STM32 shows up on USB as `0483:df11`. The nRF51 keeps running its firmware, so RESET into firmware brings the STM32 back up in its firmware. Implemented by the nRF51 firmware only, so it is only received over the radio. No result will be sent.

### Flashing several Crazyflies at once

With protocol version 0x11 the same image can be sent to several Crazyflies at the same time:

1. Restart each Crazyflie into its bootloader and check that both targets report version 0x11 or higher.
2. Send SET_BROADCAST_ADDRESS to target 0xFE of each Crazyflie, on its own address. Use the same broadcast address for all of them.
3. Send LOAD_BUFFER and WRITE_FLASH, for either target, to the broadcast address. Nothing is answered, so instead of polling FLASH_STATUS wait for the worst case erase and programming time of the pages written before sending the next chunk.
4. Verify each Crazyflie on its own address with RANGE_CRC over the whole image, or with PAGE_CRC to find the pages that differ, and flash those again the normal way.

To speed up the steps done one Crazyflie at a time, such as the verification or writing data that differs per Crazyflie, SET_CHANNEL can spread the Crazyflies over several channels, each served by its own radio.

## Contribute
Go to the [contribute page](https://www.bitcraze.io/contribute/) on our website to learn more.

### Test code for contribution
Run the automated build locally to test your code

	./tools/build/build

