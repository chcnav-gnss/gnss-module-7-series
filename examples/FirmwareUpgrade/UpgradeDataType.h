/**********************************************************************//**
			Firmware Upgrade

			Upgrade Module
*-
@file		UpgradeDataType.h
@copyright	CHCNAV
@author		luoshuaitao
@date		2024/02/20
@brief

**************************************************************************/
#ifndef _UPGRADE_DATA_TYPE_H_
#define _UPGRADE_DATA_TYPE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define UPGRADE_DEVICE_NUM_MAX			(24u)

typedef enum _UPDATE_PROGRESS_E
{
	UPDATE_PROGRESS_EXIT = 0,				/**< Not entering the upgrade process */
	UPDATE_PROGRESS_STARTED,				/**< Started upgrade process */
	UPDATE_PROGRESS_WRITED,					/**< Overwrite completed */
	UPDATE_PROGRESS_MCU_BOOTABLE,			/**< MCU bootable */
	UPDATE_PROGRESS_SUCCESSFUL,				/**< Firmware update completed */
	UPDATE_PROGRESS_FAILED_RETRY,			/**< MCU upgrade is abnormal and can be restored by re upgrading */
	UPDATE_PROGRESS_FAILED_DAMAGED,			/**< MCU upgrade is abnormal and cannot be restored.
												The flash memory is damaged and needs to be returned to the factory */
} UPDATE_PROGRESS_E;

#if !defined( __GCC__)
#pragma pack(push,1)
#endif

typedef struct _DEVICE_INFO_T
{
	unsigned char Product[64];		/**< Product name */
	unsigned char HardWareVer[64];	/**< HardWare version */
	unsigned char SoftWareVer[64];	/**< SotfWare version */
} DEVICE_INFO_T;

typedef struct _FILE_INFO_T
{
	unsigned int FileSize;		/**< The byte size of the firmware file, including the file information itself */
	unsigned int FileInfoSize;	/**< The byte size of firmware file information. The structure of firmware file information allows for changes,
									so the size may change. The function of this field is to inform the server of the byte size of the file information,
									making it convenient to send the data with the specified byte size at the beginning of the firmware file to 
									the client when requesting device upgrade */
	unsigned char FWNum;		/**< The number of microcontroller partitions contained in the firmware file, each containing a firmware and firmware
									information describing the firmware. If there are microcontroller partitions, it may be necessary for the client
									to restart and enter BootLoader for upgrading */
	unsigned char DevNum;		/**< The number of device partitions contained in the firmware file, with each partition storing one firmware and
									one firmware information describing the firmware. Each partition corresponds to one attached device */
	unsigned char Pad[2];		/**< Fill in fields: 4-byte alignment */
} FILE_INFO_T;

typedef struct _PARTITION_INFO_T
{
	unsigned int Addr[UPGRADE_DEVICE_NUM_MAX];	/**< The offset address of each partition relative to the beginning of the firmware file requires 4-byte alignment.
													The beginning of the partition contains firmware information, followed by firmware data in BIN format */
	unsigned int Size[UPGRADE_DEVICE_NUM_MAX];	/**< The byte size of each partition requires 4-byte alignment */
} PARTITION_INFO_T;

typedef struct _FIRMWARE_INFO_T
{
	unsigned char ProductModel[64];		/**< Product model */
	unsigned char Version[64];			/**< Firmware software version */
	unsigned char MD5[16];				/**< Firmware MD5 */
	unsigned int ExeAddr;				/**< The execution address of the firmware. This field is valid for microcontroller
											firmware and invalid for firmware of attached devices */
	unsigned int Size;					/**< The byte length of the firmware must not be greater than the partition size
											in the partition information minus the size of the firmware information */
} FIRMWARE_INFO_T;

#if !defined( __GCC__)
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif  /** _UPGRADE_DATA_TYPE_H_ */

