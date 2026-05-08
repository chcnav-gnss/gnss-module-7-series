/**********************************************************************//**
			Uart Common

			Uart Module
*-
@file		UartCommon.h
@copyright	CHCNAV
@author		luoshuaitao
@date		2024/02/20
@brief

**************************************************************************/
#ifndef _UART_COMMON_H_
#define _UART_COMMON_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

int UartOpen(const char* pDevName, int Mode);
int UartOptionSet(int fd, int Speed, unsigned char Bits, unsigned char Stop, char Parity);
int UartRead(int fd, char* pBuff, int Size);
int UartWrite(int fd, char* pBuff, int Size);
int UartBlockMode(int fd, int Mode);
int UartClose(int fd);

#ifdef __cplusplus
}
#endif

#endif  /** _UART_COMMON_H_ */

