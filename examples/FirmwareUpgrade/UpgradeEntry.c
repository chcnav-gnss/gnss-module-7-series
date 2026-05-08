/**********************************************************************//**
			Firmware Upgrade

			Upgrade Module
*-
@file		UpgradeEntry.c
@copyright	CHCNAV
@author		luoshuaitao
@date		2024/02/20
@brief

**************************************************************************/
#include <stddef.h>
#include <stdio.h>
#include<string.h>
#include "UpgradeEntry.h"

/**********************************************************************//**
@brief  get upgrade frame max length encode handle

@param pFrameHandle		[In] frame handle

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
static int GetFrameMaxLenEncodeHandle(FRAME_HANDLE_T* pFrameHandle)
{
	if (!pFrameHandle)
	{
		return -1;
	}

	UpgradeFrameEncode(pFrameHandle, NULL, 0, FRAME_ACTION_SEND, 0, CHC_CMD_GET_DEVICE_MAX_FRAME_LEN);

	return 0;
}

/**********************************************************************//**
@brief  handle send frame max length encode handle

@param pFrameHandle		[In] frame handle
@param pData			[In] pointer to send data buffer
@param DataLen			[In] data len

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
static int SendFrameMaxLenEncodeHandle(FRAME_HANDLE_T* pFrameHandle, unsigned char* pData, unsigned int DataLen)
{
	if ((!pFrameHandle) || (!pData) || (DataLen == 0))
	{
		return -1;
	}

	UpgradeFrameEncode(pFrameHandle, pData, DataLen, FRAME_ACTION_SEND, 0, CHC_CMD_SET_SEND_MAX_FRAME_LEN);

	return 0;
}

/**********************************************************************//**
@brief  get device information encode handle

@param pFrameHandle		[In] frame handle

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
static int GetDeviceInfoEncodeHandle(FRAME_HANDLE_T* pFrameHandle)
{
	if (!pFrameHandle)
	{
		return -1;
	}

	UpgradeFrameEncode(pFrameHandle, NULL, 0, FRAME_ACTION_SEND, 0, CHC_CMD_UPDATE_GET_DEVICE_INFO);

	return 0;
}

/**********************************************************************//**
@brief  handle upgrade request encode handle

@param pFrameHandle		[In] frame handle
@param pData			[In] pointer to send data buffer
@param DataLen			[In] data len

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
static int RequestUpgradeEncodeHandle(FRAME_HANDLE_T* pFrameHandle, unsigned char* pData, unsigned int DataLen)
{
	if ((!pFrameHandle) || (!pData) || (DataLen == 0))
	{
		return -1;
	}

	UpgradeFrameEncode(pFrameHandle, pData, DataLen, FRAME_ACTION_SEND, 0, CHC_CMD_UPDATE_REQUEST_UPGRADE);

	return 0;
}

/**********************************************************************//**
@brief  get device status encode handle

@param pFrameHandle		[In] frame handle

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
static int GetDeviceStatusEncodeHandle(FRAME_HANDLE_T* pFrameHandle)
{
	if (!pFrameHandle)
	{
		return -1;
	}

	UpgradeFrameEncode(pFrameHandle, NULL, 0, FRAME_ACTION_SEND, 0, CHC_CMD_GET_DEVICE_STATUS);

	return 0;
}

/**********************************************************************//**
@brief  get upgrade update status encode handle

@param pFrameHandle		[In] frame handle

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
static int GetUpdateStatusEncodeHandle(FRAME_HANDLE_T* pFrameHandle)
{
	if (!pFrameHandle)
	{
		return -1;
	}

	UpgradeFrameEncode(pFrameHandle, NULL, 0, FRAME_ACTION_SEND, 0, CHC_CMD_UPDATE_GET_UPDATE_STATUS);

	return 0;
}

/**********************************************************************//**
@brief  Send upgrade firmware encode handle

@param pFrameHandle		[In] frame handle
@param pData			[In] pointer to send data buffer
@param DataLen			[In] data len

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
static int SendFirmwareEncodeHandle(FRAME_HANDLE_T* pFrameHandle, unsigned char* pData, unsigned int DataLen)
{
	int Result = -1;

	if (!pFrameHandle)
	{
		return -1;
	}

	if ((pData == NULL) || (DataLen == 0))
	{
		Result = UpgradeFrameEncode(pFrameHandle, NULL, 0, FRAME_ACTION_SEND, 0, CHC_CMD_UPDATE_SEND_FIRMWARE);
	}
	else
	{
		Result = UpgradeFrameEncode(pFrameHandle, pData, DataLen, FRAME_ACTION_SEND, 0, CHC_CMD_UPDATE_SEND_FIRMWARE);
	}

	return Result;
}

/**********************************************************************//**
@brief  request reboot encode handle

@param pFrameHandle		[In] frame handle

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
static int RequestRebootEncodeHandle(FRAME_HANDLE_T* pFrameHandle)
{
	if (!pFrameHandle)
	{
		return -1;
	}

	UpgradeFrameEncode(pFrameHandle, NULL, 0, FRAME_ACTION_SEND, 0, CHC_CMD_UPDATE_REQUEST_REBOOT);

	return 0;
}

/**********************************************************************//**
@brief  set fast update mode encode handle

@param pFrameHandle		[In] frame handle

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
static int SetFastUpdateModeEncodeHandle(FRAME_HANDLE_T* pFrameHandle)
{
	if (!pFrameHandle)
	{
		return -1;
	}

	UpgradeFrameEncode(pFrameHandle, NULL, 0, FRAME_ACTION_SEND, 0, CHC_CMD_SET_FAST_UPDATE_MODE);

	return 0;
}

/**********************************************************************//**
@brief  handle set lora update transfer mode encode handle

@param pFrameHandle		[In] frame handle
@param pData			[In] pointer to send data buffer
@param DataLen			[In] data len

@retval <0 error, =0 success

@author luoshuaitao
@date 2025/10/11
@note
**************************************************************************/
static int SetRadioUpdateTransferModeEncodeHandle(FRAME_HANDLE_T* pFrameHandle, unsigned char* pData, unsigned int DataLen)
{
	if ((!pFrameHandle) || (!pData) || (DataLen == 0))
	{
		return -1;
	}

	UpgradeFrameEncode(pFrameHandle, pData, DataLen, FRAME_ACTION_SEND, 0, CHC_CMD_SET_RADIO_UPDATE_TRANSFER_MODE);

	return 0;
}

/**********************************************************************//**
@brief  get upgrade frame max length reply handle

@param pFrameHandle		[In] frame handle
@param pFrameMaxLen		[Out] frame max length

@retval <0 error, =0 success

@author luoshuaitao
@date 2023/02/21
@note
**************************************************************************/
int GetFrameMaxLenReplyHandle(FRAME_HANDLE_T *pFrameHandle, int* pFrameMaxLen)
{
	unsigned char* pBuf;
	int FrameMaxLen = 0;

	if ((!pFrameHandle) || (!pFrameMaxLen))
	{
		return -1;
	}

	if (pFrameHandle->FrameRecv.Cmd.ActionStatus != 0) /**< action status check */
	{
		return -1;
	}

	if (pFrameHandle->FrameRecv.FrameData.DataCount != 4) /**< length check (frame vaild check) */
	{
		return -1;
	}

	pBuf = pFrameHandle->FrameRecv.FrameData.pFrameData;

	FrameMaxLen = ((unsigned int)pBuf[3] << 24) + ((unsigned int)pBuf[2] << 16) + ((unsigned int)pBuf[1] << 8) + ((unsigned int)pBuf[0]);
	if ((FrameMaxLen < 1024) || (FrameMaxLen > UPGRADE_PROTOCOL_LENGTH_MAX))
	{
		return -1;
	}

	*pFrameMaxLen = FrameMaxLen;

	return 0;
}

/**********************************************************************//**
@brief  handle send frame max length reply handle

@param pFrameHandle		[In] frame handle

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
int SendFrameMaxLenReplyHandle(FRAME_HANDLE_T *pFrameHandle)
{
	unsigned char* pBuf;

	if (!pFrameHandle)
	{
		return -1;
	}

	if (pFrameHandle->FrameRecv.Cmd.ActionStatus != 0) /**< action status check */
	{
		return -1;
	}

	if (pFrameHandle->FrameRecv.FrameData.DataCount != 1) /**< length check (frame vaild check) */
	{
		return -1;
	}

	pBuf = pFrameHandle->FrameRecv.FrameData.pFrameData;
	if (pBuf[0] != UPGRADE_REPLY_SUCCESS)
	{
		return -1;
	}

	return 0;
}

/**********************************************************************//**
@brief  get device information reply handle

@param pFrameHandle		[In] frame handle
@param pDeviceInfo		[Out] device information

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
int GetDeviceInfoReplyHandle(FRAME_HANDLE_T *pFrameHandle, DEVICE_INFO_T* pDeviceInfo)
{
	if (!pFrameHandle)
	{
		return -1;
	}

	if (pFrameHandle->FrameRecv.Cmd.ActionStatus != 0) /**< action status check */
	{
		return -1;
	}

	if (pFrameHandle->FrameRecv.FrameData.DataCount != sizeof(DEVICE_INFO_T)) /**< length check (frame vaild check) */
	{
		return -1;
	}

	pDeviceInfo = (DEVICE_INFO_T*)pFrameHandle->FrameRecv.FrameData.pFrameData;

	memcpy(pDeviceInfo, pDeviceInfo, sizeof(DEVICE_INFO_T));

	return 0;
}

/**********************************************************************//**
@brief  handle upgrade request reply handle

@param pFrameHandle		[In] frame handle

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
int RequestUpgradeReplyHandle(FRAME_HANDLE_T *pFrameHandle)
{
	unsigned char* pBuf;

	if (!pFrameHandle)
	{
		return -1;
	}

	if (pFrameHandle->FrameRecv.Cmd.ActionStatus != 0) /**< action status check */
	{
		return -1;
	}

	if (pFrameHandle->FrameRecv.FrameData.DataCount != 1) /**< length check (frame vaild check) */
	{
		return -1;
	}

	pBuf = pFrameHandle->FrameRecv.FrameData.pFrameData;
	if (pBuf[0] != UPGRADE_REQUEST_STATUS_ALLOW)
	{
		return -1;
	}

	return 0;
}

/**********************************************************************//**
@brief  get device status reply handle

@param pFrameHandle		[In] frame handle

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
int GetDeviceStatusReplyHandle(FRAME_HANDLE_T *pFrameHandle)
{
	unsigned char* pBuf;

	if (!pFrameHandle)
	{
		return -1;
	}

	if (pFrameHandle->FrameRecv.Cmd.ActionStatus != 0) /**< action status check */
	{
		return -1;
	}

	if (pFrameHandle->FrameRecv.FrameData.DataCount != 1) /**< length check (frame vaild check) */
	{
		return -1;
	}

	pBuf = pFrameHandle->FrameRecv.FrameData.pFrameData;
	if (pBuf[0] != REPLY_UPGRADE_STATUS_UPDATING)
	{
		return -1;
	}

	return 0;
}

/**********************************************************************//**
@brief  get upgrade update status reply handle

@param pFrameHandle		[In] frame handle
@param UpgradeStatus	[In] Upgrade status

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
int GetUpdateStatusReplyHandle(FRAME_HANDLE_T *pFrameHandle, unsigned int UpgradeStatus)
{
	unsigned char* pBuf;

	if (!pFrameHandle)
	{
		return -1;
	}

	if (pFrameHandle->FrameRecv.Cmd.ActionStatus != 0) /**< action status check */
	{
		return -1;
	}

	if (pFrameHandle->FrameRecv.FrameData.DataCount != 1) /**< length check (frame vaild check) */
	{
		return -1;
	}

	pBuf = pFrameHandle->FrameRecv.FrameData.pFrameData;
	if (pBuf[0] != UpgradeStatus)
	{
		return -1;
	}

	return 0;
}

/**********************************************************************//**
@brief  send Sub Package upgrade firmware reply handle

@param pFrameHandle		[In] frame handle

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
int SendSubPackageFirmwareReplyHandle(FRAME_HANDLE_T *pFrameHandle, unsigned int FrameMaxLen, unsigned int* pFileAddr, unsigned int* pFileSize)
{
	unsigned char* pBuf;
	unsigned int FileAddr;
	unsigned int FileSize;

	if ((!pFrameHandle) || (!pFileAddr) || (!pFileSize))
	{
		return -1;
	}

	if (pFrameHandle->FrameRecv.Cmd.ActionStatus != 0) /**< action status check */
	{
		return -1;
	}

	if (pFrameHandle->FrameRecv.FrameData.DataCount != 8) /**< length check (frame vaild check) */
	{
		return -1;
	}

	pBuf = pFrameHandle->FrameRecv.FrameData.pFrameData;

	FileAddr = ((unsigned int)pBuf[3] << 24) + ((unsigned int)pBuf[2] << 16) + ((unsigned int)pBuf[1] << 8) + ((unsigned int)pBuf[0]);
	FileSize = ((unsigned int)pBuf[7] << 24) + ((unsigned int)pBuf[6] << 16) + ((unsigned int)pBuf[5] << 8) + ((unsigned int)pBuf[4]);
	if ((FileSize > (FrameMaxLen - UPGRADE_PROTOCOL_FILED_LEN)))
	{
		return -1;
	}

	*pFileAddr = FileAddr;
	*pFileSize = FileSize;

	return 0;
}

/**********************************************************************//**
@brief  send full Package upgrade firmware reply handle

@param pFrameHandle		[In] frame handle

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
int SendFullPackageFirmwareReplyHandle(FRAME_HANDLE_T *pFrameHandle)
{
	unsigned char* pBuf;

	if (!pFrameHandle)
	{
		return -1;
	}

	if (pFrameHandle->FrameRecv.Cmd.ActionStatus != 0) /**< action status check */
	{
		return -1;
	}

	if (pFrameHandle->FrameRecv.FrameData.DataCount != 1) /**< length check (frame vaild check) */
	{
		return -1;
	}

	pBuf = pFrameHandle->FrameRecv.FrameData.pFrameData;
	if (pBuf[0] != UPGRADE_REPLY_SUCCESS)
	{
		return -1;
	}

	return 0;
}

/**********************************************************************//**
@brief  handle request reboot reply handle

@param pFrameHandle		[In] frame handle

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
int RequestRebootReplyHandle(FRAME_HANDLE_T *pFrameHandle)
{
	unsigned char* pBuf;

	if (!pFrameHandle)
	{
		return -1;
	}

	if (pFrameHandle->FrameRecv.Cmd.ActionStatus != 0) /**< action status check */
	{
		return -1;
	}

	if (pFrameHandle->FrameRecv.FrameData.DataCount != 1) /**< length check (frame vaild check) */
	{
		return -1;
	}

	pBuf = pFrameHandle->FrameRecv.FrameData.pFrameData;
	if (pBuf[0] != UPGRADE_REPLY_SUCCESS)
	{
		return -1;
	}

	return 0;
}

/**********************************************************************//**
@brief  handle set fast update mode reply handle

@param pFrameHandle		[In] frame handle

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
int SetFastUpdateModeReplyHandle(FRAME_HANDLE_T *pFrameHandle)
{
	unsigned char* pBuf;

	if (!pFrameHandle)
	{
		return -1;
	}

	if (pFrameHandle->FrameRecv.Cmd.ActionStatus != 0) /**< action status check */
	{
		return -1;
	}

	if (pFrameHandle->FrameRecv.FrameData.DataCount != 1) /**< length check (frame vaild check) */
	{
		return -1;
	}

	pBuf = pFrameHandle->FrameRecv.FrameData.pFrameData;
	if (pBuf[0] != UPGRADE_REPLY_SUCCESS)
	{
		return -1;
	}

	return 0;
}

/**********************************************************************//**
@brief  handle set lora update transfer mode reply handle

@param pFrameHandle		[In] frame handle

@retval <0 error, =0 success

@author luoshuaitao
@date 2025/10/11
@note
**************************************************************************/
int SetRadioUpdateTransferModeReplyHandle(FRAME_HANDLE_T *pFrameHandle)
{
	unsigned char* pBuf;

	if (!pFrameHandle)
	{
		return -1;
	}

	if (pFrameHandle->FrameRecv.Cmd.ActionStatus != 0) /**< action status check */
	{
		return -1;
	}

	if (pFrameHandle->FrameRecv.FrameData.DataCount != 1) /**< length check (frame vaild check) */
	{
		return -1;
	}

	pBuf = pFrameHandle->FrameRecv.FrameData.pFrameData;
	if (pBuf[0] != UPGRADE_REPLY_SUCCESS)
	{
		return -1;
	}

	return 0;
}

/**********************************************************************//**
@brief  upgrade frame encode

@param pFrameHandle		[In] frame handle
@param CmdID			[In] Command id

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
int UpgradeFrameDataEncode(FRAME_HANDLE_T *pFrameHandle, unsigned int CmdID, unsigned char* pData, unsigned int DataLen)
{
	int Result = -1;

	if (!pFrameHandle)
	{
		return -1;
	}

	switch (CmdID)
	{
		case CHC_CMD_GET_DEVICE_MAX_FRAME_LEN:				/**< get device frame max length */
			Result = GetFrameMaxLenEncodeHandle(pFrameHandle);
			break;
		case CHC_CMD_SET_SEND_MAX_FRAME_LEN:				/**< get device frame max length */
			Result = SendFrameMaxLenEncodeHandle(pFrameHandle, pData, DataLen);
			break;
		case CHC_CMD_UPDATE_GET_DEVICE_INFO:				/**< get device information */
			Result = GetDeviceInfoEncodeHandle(pFrameHandle);
			break;
		case CHC_CMD_UPDATE_REQUEST_UPGRADE:				/**< request upgrade */
			Result = RequestUpgradeEncodeHandle(pFrameHandle, pData, DataLen);
			break;
		case CHC_CMD_GET_DEVICE_STATUS:						/**< get device run status */
			Result = GetDeviceStatusEncodeHandle(pFrameHandle);
			break;
		case CHC_CMD_UPDATE_GET_UPDATE_STATUS:				/**< get update status */
			Result = GetUpdateStatusEncodeHandle(pFrameHandle);
			break;
		case CHC_CMD_UPDATE_SEND_FIRMWARE:					/**< transport firmware */
			Result = SendFirmwareEncodeHandle(pFrameHandle, pData, DataLen);
			break;
		case CHC_CMD_UPDATE_REQUEST_REBOOT:				/**< get device frame max length */
			Result = RequestRebootEncodeHandle(pFrameHandle);
			break;
		case CHC_CMD_SET_FAST_UPDATE_MODE:				/**< get device frame max length */
			Result = SetFastUpdateModeEncodeHandle(pFrameHandle);
			break;
		case CHC_CMD_SET_RADIO_UPDATE_TRANSFER_MODE:		/**< set radio update transfer mode */
			Result = SetRadioUpdateTransferModeEncodeHandle(pFrameHandle, pData, DataLen);
			break;
		default:
			break;
	}

	return Result;
}

