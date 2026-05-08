/**********************************************************************//**
			Firmware Upgrade

			Upgrade Module
*-
@file		CRCGenerate.h
@copyright	CHCNAV
@author		luoshuaitao
@date		2024/02/20
@brief

**************************************************************************/
#ifndef __CRC_GENERATE_H__
#define __CRC_GENERATE_H__

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

#define CRC32				(1)	/**< enable CRC32 check */

#define CRC16_MODBUS		(1)	/**< enable CRC16 MODBUS check */

unsigned short CRC16ModbusGenerate(unsigned char* pBuf, unsigned char Len);
unsigned int CRC32Generate( const unsigned char* pBuf, unsigned int Len);


#if defined(__cplusplus)
}
#endif

#endif  /* __CRC_GENERATE_H__ */

