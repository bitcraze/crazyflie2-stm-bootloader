#ifndef __CRC32_H__
#define __CRC32_H__

#include <stdint.h>

/**
 * Compute standard CRC-32 (ISO 3309) of a data buffer.
 *
 * Polynomial 0xEDB88320 (reflected), init 0xFFFFFFFF, final XOR 0xFFFFFFFF.
 * Compatible with the nRF51 firmware and bootloader CRC32 implementations.
 */
uint32_t crc32Calculate(const void *buffer, unsigned int size);

#endif /* __CRC32_H__ */
