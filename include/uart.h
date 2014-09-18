#ifndef __UART_H__
#define __UART_H__

#include <stdbool.h>

void uartInit();
bool uartIsRxReady();
char uartGetc();
bool uartIsTxReady();
void uartPutc(char data);
void uartPuts(char *string);

#endif
