/**********************************************************************//**
			Firmware Upgrade

			Upgrade Module
*-
@file		Upgrade.h
@copyright	CHCNAV
@author		luoshuaitao
@date		2024/08/16
@brief

**************************************************************************/
#ifndef __UPGRADE_H__
#define __UPGRADE_H__

#if defined(__cplusplus)
	extern "C" {
#endif

#include "UpgradeProtocol.h"
#include "UpgradeDataType.h"
#include "UpgradeDataStruct.h"

#define UPGRADE_MODE_SUB_PACKAGE		0
#define UPGRADE_MODE_FULL_PACKAGE		1


#define UPGRADE_PROTOCOL_DATA_MAX_LEN_4K			(4096U)
#define UPGRADE_PROTOCOL_DATA_MAX_LEN_8K			(8192U)
#define UPGRADE_PROTOCOL_DATA_MAX_LEN_16K			(16384U)
#define UPGRADE_PROTOCOL_DATA_MAX_LEN_32K			(32768U)

#define UPGRADE_PROTOCOL_MAX_DATA_LEN				UPGRADE_PROTOCOL_DATA_MAX_LEN_32K

#define UPGRADE_PROTOCOL_MAX_FRAME_LEN				(UPGRADE_PROTOCOL_MAX_DATA_LEN+UPGRADE_PROTOCOL_FILED_LEN)

void SetDebugPrintf(int DebugPrintf);

int FitBaudrate(const char *pDevName, int Baudrate);
int UpgradeM7xx(CMD_ARGS_T* pCmdArgs);

#if defined(__cplusplus)
}
#endif

#endif  /**< __UPGRADE_H__ */

