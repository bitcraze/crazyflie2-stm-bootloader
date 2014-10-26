#ifndef __BOOTPIN_H__
#define __BOOTPIN_H__

#include <stdbool.h>

void bootpinInit(void);

void bootpinDeinit(void);

bool bootpinStartFirmware(void);

bool bootpinStartBootloader(void);

bool bootpinNrfReset(void);

#endif //__BOOTPIN_H__
