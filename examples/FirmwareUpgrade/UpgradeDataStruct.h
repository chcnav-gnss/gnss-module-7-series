/**********************************************************************//**
			Firmware Upgrade

			Upgrade Module
*-
@file		UpgradeDataStruct.h
@copyright	CHCNAV
@author		luoshuaitao
@date		2024/09/03
@brief

**************************************************************************/
#ifndef _UPGRADE_DATA_STRUCT_H_
#define _UPGRADE_DATA_STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <linux/limits.h>

#define DEVICE_PATH_LEN_MAX		(PATH_MAX)
#define BAUDRATE_LEN_MAX		(32u)
#define PKG_PATH_LEN_MAX		(PATH_MAX)

#define SUCCESS					0
#define ERROR_OPEN				-1
#define ERROR_READ_HEADER		-2
#define ERROR_READ_BODY			-3
#define ERROR_CRC				-4
#define ERROR_MALLOC			-5
#define ERROR_STAT				-6
#define ERROR_SUM32				-7
#define ERROR_GET_ARGS			-8

#define ERROR_OPEN_SERIAL				-101
#define ERROR_BAUDRATE_NOT_SUPPORT		-102
#define ERROR_TCGETATTR_ERROR			-103
#define ERROR_TCSETATTR_ERROR			-104

typedef struct _CMD_ARGS_T
{
	char Device[DEVICE_PATH_LEN_MAX];	/**< device path */
	int Baudrate;						/**< device baudrate */
	int TransferSize;					/**< protocol data body transfer size */
	int DebugOutput;					/**< debug output */
	int AutoReset;						/**< auto reset */
	int FastUpdate;						/**< fast update by close BB, 0:disable, 1:enable*/
	int ForceUpdate;					/**< force update, 0:disable, 1:enable */
	int RemoteDeviceUpdate;				/**< remote device update */
	int UpdateMode;						/**< update mode, 0:sub update, 1:full update */
	char PkgPath[PKG_PATH_LEN_MAX];		/**< firmware package path */
} CMD_ARGS_T;

#ifdef __cplusplus
}
#endif

#endif  /** _UPGRADE_DATA_STRUCT_H_ */

