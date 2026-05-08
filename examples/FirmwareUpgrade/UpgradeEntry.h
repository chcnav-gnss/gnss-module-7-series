/**********************************************************************//**
			Firmware Upgrade

			Upgrade Module
*-
@file		UpgradeEntry.h
@copyright	CHCNAV
@author		luoshuaitao
@date		2024/02/20
@brief

**************************************************************************/
#ifndef __UPGRADE_ENTRY_H__
#define __UPGRADE_ENTRY_H__

#if defined(__cplusplus)
	extern "C" {
#endif

#include "UpgradeProtocol.h"
#include "UpgradeDataType.h"

typedef enum _UPGRADE_STATUS_E
{
	UPGRADE_STATUS_IDLE = 0,							/**< Upgrade idle */
	UPGRADE_STATUS_GET_DEVICE_MAX_FRAME_LEN,			/**< Get the maximum frame data length supported by the device */
	UPGRADE_STATUS_SEND_MAX_FRAME_LEN,					/**< Send the maximum frame data length supported by the device */
	UPGRADE_STATUS_GET_DEVICE_INFO,						/**< Get device information */
	UPGRADE_STATUS_REQUEST_UPGRADE,						/**< Request upgrade */
	UPGRADE_STATUS_SET_FAST_UPDATE_MODE,				/**< Set fast update mode by close bb interrupt */
	UPGRADE_STATUS_GET_DEVICE_STATUS,					/**< Get device status */
	UPGRADE_STATUS_UPDATE_SEND_FIRMWARE,				/**< Send firmware */
	UPGRADE_STATUS_GET_UPDATE_STATUS,					/**< Get update status */
	UPGRADE_STATUS_REQUEST_REBOOT,						/**< Request reboot */
	UPGRADE_STATUS_FAIL,								/**< Upgrade fail */
	UPGRADE_STATUS_SUCCESS,								/**< Upgrade success */
	UPGRADE_STATUS_OPEN_RADIO_UPDATE_TRANSFER_MODE, /**< Set open radio update transfer mode */
	UPGRADE_STATUS_CLOSE_RADIO_UPDATE_TRANSFER_MODE, /**< Set close radio update transfer mode */
} UPGRADE_STATUS_E;

typedef enum _UPGRADE_REQUEST_STATUS_E
{
	UPGRADE_REQUEST_STATUS_REJECT = 0,		/**< reject upgrade */
	UPGRADE_REQUEST_STATUS_ALLOW = 1,		/**< allow upgrade */
} UPGRADE_REQUEST_STATUS_E;

typedef enum _REPLY_UPGRADE_STATUS_E
{
	REPLY_UPGRADE_STATUS_IDLE = 0,			/**< idle */
	REPLY_UPGRADE_STATUS_UPDATING = 1,		/**< updating */
	REPLY_UPGRADE_STATUS_SUCCESS = 2,		/**< update success */
	REPLY_UPGRADE_STATUS_FAIL = 3,			/**< update fail */
} REPLY_UPGRADET_STATUS_E;

typedef enum _UPGRADE_REPLY_E
{
	UPGRADE_REPLY_FAIL = 0, 	/**< upgrade reply fail */
	UPGRADE_REPLY_SUCCESS = 1, 	/**< upgrade reply success */
} UPGRADE_REPLY_E;

typedef enum _RADIO_UPDATE_TRANSFER_MODE_E
{
	RADIO_UPDATE_TRANSFER_MODE_CLOSE = 0,	/**< close lora update transfer mode */
	RADIO_UPDATE_TRANSFER_MODE_OPEN = 1,		/**< open lora update transfer mode */
} RADIO_UPDATE_TRANSFER_MODE_E;

typedef struct _UPDATE_TRANSFER_MODE_INFO_T
{
	unsigned char TransferMode; 
	unsigned char ChannelIndex;
} UPDATE_TRANSFER_MODE_INFO_T;

int UpgradeFrameDataEncode(FRAME_HANDLE_T *pFrameHandle, unsigned int CmdID, unsigned char* pData, unsigned int DataLen);

int GetFrameMaxLenReplyHandle(FRAME_HANDLE_T *pFrameHandle, int* pFrameMaxLen);
int SendFrameMaxLenReplyHandle(FRAME_HANDLE_T *pFrameHandle);
int GetDeviceInfoReplyHandle(FRAME_HANDLE_T *pFrameHandle, DEVICE_INFO_T* pDeviceInfo);
int RequestUpgradeReplyHandle(FRAME_HANDLE_T *pFrameHandle);
int GetDeviceStatusReplyHandle(FRAME_HANDLE_T *pFrameHandle);
int GetUpdateStatusReplyHandle(FRAME_HANDLE_T *pFrameHandle, unsigned int UpgradeStatus);
int SendSubPackageFirmwareReplyHandle(FRAME_HANDLE_T *pFrameHandle, unsigned int FrameMaxLen, unsigned int* pFileAddr, unsigned int* pFileSize);
int SendFullPackageFirmwareReplyHandle(FRAME_HANDLE_T *pFrameHandle);
int RequestRebootReplyHandle(FRAME_HANDLE_T *pFrameHandle);
int SetFastUpdateModeReplyHandle(FRAME_HANDLE_T *pFrameHandle);
int SetRadioUpdateTransferModeReplyHandle(FRAME_HANDLE_T *pFrameHandle);

#if defined(__cplusplus)
}
#endif

#endif  /**< __UPGRADE_ENTRY_H__ */

