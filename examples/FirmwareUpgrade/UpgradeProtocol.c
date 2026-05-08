/**********************************************************************//**
			Firmware Upgrade

			Upgrade Module
*-
@file		UpgradeProtocol.c
@copyright	CHCNAV
@author		luoshuaitao
@date		2024/02/20
@brief

**************************************************************************/
#include <string.h>
#include <stdio.h>

#include "UpgradeProtocol.h"
#include "CRCGenerate.h"

/**********************************************************************//**
@brief  Upgrade frame to string

@param pProto			[In] Pointer to the frame structure to be converted
@param pFrameStr		[Out] Converted data flow

@author luoshuaitao
@date 2024/02/20
@note
**************************************************************************/
static void UpgradeFrame2String(PROTOCOL_FRAME_T* pProto, FRAME_STRING_T* pFrameStr)
{
	unsigned short StrAddr;

	StrAddr = sizeof(pProto->Header) + sizeof(pProto->Version) + sizeof(pProto->FrameNumb);
	memcpy(pFrameStr->StrBuf, (unsigned char *)pProto, StrAddr);

	pFrameStr->StrBuf[StrAddr++] = (pProto->Length >> 8) & 0x00FF;
	pFrameStr->StrBuf[StrAddr++] = (pProto->Length) & 0x00FF;

	pFrameStr->StrBuf[StrAddr++] = pProto->SrcAddr;
	pFrameStr->StrBuf[StrAddr++] = pProto->DstAddr;

	pFrameStr->StrBuf[StrAddr++] = (pProto->Field >> 24) & 0xFFU;
	pFrameStr->StrBuf[StrAddr++] = (pProto->Field >> 16) & 0xFFU;
	pFrameStr->StrBuf[StrAddr++] = (pProto->Field >> 8) & 0xFFU;
	pFrameStr->StrBuf[StrAddr++] = (pProto->Field) & 0xFFU;

	StrAddr += pProto->FrameData.DataCount;

	pFrameStr->StrBuf[StrAddr++] = (pProto->CRC >> 24) & 0xFFU;
	pFrameStr->StrBuf[StrAddr++] = (pProto->CRC >> 16) & 0xFFU;
	pFrameStr->StrBuf[StrAddr++] = (pProto->CRC >> 8) & 0xFFU;
	pFrameStr->StrBuf[StrAddr++] = (pProto->CRC) & 0xFFU;

	pFrameStr->Len = StrAddr;
}

/**********************************************************************//**
@brief  Upgrade frame to string add CRC

@param pProto			[In] Pointer to the frame structure to be converted
@param pFrameStr		[Out] Converted data flow

@author luoshuaitao
@date 2024/02/20
@note
**************************************************************************/
static void UpgradeFrame2StringAddCRC(PROTOCOL_FRAME_T* pProto, FRAME_STRING_T* pFrameStr)
{
	unsigned short StrAddr;

	StrAddr = pFrameStr->Len - sizeof(pProto->CRC);

	pFrameStr->StrBuf[StrAddr++] = (pProto->CRC >> 24) & 0xFFU;
	pFrameStr->StrBuf[StrAddr++] = (pProto->CRC >> 16) & 0xFFU;
	pFrameStr->StrBuf[StrAddr++] = (pProto->CRC >> 8) & 0xFFU;
	pFrameStr->StrBuf[StrAddr++] = (pProto->CRC) & 0xFFU;
}

/**********************************************************************//**
@brief  Upgrade string to frame

@param pProto			[Out] Pointer to the frame structure to be converted
@param pBuf				[In] Pointer to data flow
@param Length			[In] Data length

@author luoshuaitao
@date 2024/02/20
@note
**************************************************************************/
static void UpgradeString2Frame(PROTOCOL_FRAME_T* pProto, unsigned char* pBuf, unsigned short Length)
{
	unsigned short StrAddr;

	StrAddr = sizeof(pProto->Header) + sizeof(pProto->Version) + sizeof(pProto->FrameNumb);
	memcpy((unsigned char *)pProto, pBuf, StrAddr);

	pProto->Length = Length;
	StrAddr += sizeof(pProto->Length);

	pProto->SrcAddr = pBuf[StrAddr++];
	pProto->DstAddr = pBuf[StrAddr++];

	pProto->Field = ((unsigned int)pBuf[StrAddr] << 24) + ((unsigned int)pBuf[StrAddr+1] << 16) +
						((unsigned int)pBuf[StrAddr+2] << 8) + ((unsigned int)pBuf[StrAddr+3]);
	StrAddr += sizeof(pProto->Field);

	pProto->FrameData.DataCount = Length - UPGRADE_PROTOCOL_FILED_LEN;
	StrAddr += Length - UPGRADE_PROTOCOL_FILED_LEN;

	pProto->CRC = ((unsigned int)pBuf[StrAddr] << 24) + ((unsigned int)pBuf[StrAddr+1] << 16) +
						((unsigned int)pBuf[StrAddr+2] << 8) + ((unsigned int)pBuf[StrAddr+3]);
	StrAddr += sizeof(pProto->CRC);
}

/**********************************************************************//**
@brief  Upgrade frame search

@param pProto			[In] Pointer to the frame structure to be converted
@param pBuf				[Out] Pointer to data flow
@param Length			[Out] Data length

@retval FRAME_RESPONSE_CRC_ERROR: CRC Error
		FRAME_RESPONSE_OK: OK

@author luoshuaitao
@date 2024/02/20
@note
**************************************************************************/
static FRAME_RESPONSE_STATUS_E UpgradeFrameSearch(PROTOCOL_FRAME_T* pProto, unsigned char* pBuf, unsigned short Length)
{
	unsigned int FrameCRC;
	unsigned int Pos;
	unsigned char HeadSize, EndSize;

	HeadSize = sizeof(pProto->Header) + sizeof(pProto->Version);
	EndSize = sizeof(pProto->CRC);

	Pos = Length - EndSize;
	FrameCRC = ((unsigned int)pBuf[Pos] << 24) + ((unsigned int)pBuf[Pos+1] << 16) +
					((unsigned int)pBuf[Pos+2] << 8) + ((unsigned int)pBuf[Pos+3]);

	if(FrameCRC != CRC32Generate(&pBuf[HeadSize], Length - HeadSize - EndSize))
	{
		return FRAME_RESPONSE_CRC_ERROR;
	}

	UpgradeString2Frame(pProto, pBuf, Length);

	return FRAME_RESPONSE_OK;
}

/**********************************************************************//**
@brief  Check upgrade frame header

@param pProto			[In] Pointer to the frame structure to be converted

@retval 0: ok, -1:error

@author luoshuaitao
@date 2024/02/20
@note
**************************************************************************/
static int UpgradeFrameHeaderIsRight(PROTOCOL_FRAME_T* pProto)
{
	unsigned char Header[UPGRADE_PROTOCOL_FRAME_HEADER_LEN];

	Header[0] = (unsigned char)((UPGRADE_PROTOCOL_FRAME_HEADER >> 8) & 0xFFU);
	Header[1] = (unsigned char)((UPGRADE_PROTOCOL_FRAME_HEADER) & 0xFFU);

	if (pProto->Header[0] == Header[0] && pProto->Header[1] == Header[1])
	{
		return 0;
	}

	return -1;
}

/**********************************************************************//**
@brief  Upgrade frame init

@param pFrameHandle		[In] Frame handle
@param SrcAddr			[In] Source address
@param DstAddr			[In] Destination address

@retval see FRAME_RESPONSE_STATUS_E

@author luoshuaitao
@date 2024/02/20
@note
**************************************************************************/
FRAME_RESPONSE_STATUS_E UpgradeFrameInit(FRAME_HANDLE_T* pFrameHandle, unsigned char SrcAddr, unsigned char DstAddr)
{
	if (NULL == pFrameHandle)
	{
		return FRAME_RESPONSE_ERROR;
	}

	memset(pFrameHandle, 0, sizeof(FRAME_HANDLE_T)); /**< clear uframe at first. */

	/** set data buffer. */
	pFrameHandle->FrameSend.FrameData.pFrameData = pFrameHandle->FrameStr.StrBuf + UPGRADE_PROTOCOL_HEAD_LEN;
	pFrameHandle->FrameRecv.FrameData.pFrameData = pFrameHandle->RecvData + UPGRADE_PROTOCOL_HEAD_LEN;

	/** Set device id. */
	pFrameHandle->FrameSend.SrcAddr = SrcAddr;
	pFrameHandle->FrameSend.DstAddr = DstAddr;

	/** Set defult frame configuration. */
	pFrameHandle->FrameSend.Header[0] = (unsigned char)((UPGRADE_PROTOCOL_FRAME_HEADER >> 8) & 0xFFU);
	pFrameHandle->FrameSend.Header[1] = (unsigned char)((UPGRADE_PROTOCOL_FRAME_HEADER) & 0xFFU);
	pFrameHandle->FrameSend.Version = UPGRADE_PROTOCOL_VERSION;
	pFrameHandle->FrameSend.FrameNumb = 0;

	return FRAME_RESPONSE_OK;
}

/**********************************************************************//**
@brief  Upgrade frame encode

@param pFrameHandle		[In] Frame handle
@param pData			[In] pointer to frame data
@param DataLen			[In] Data length
@param Action			[In] Frame action
@param ActionStatus		[In] Frame action status
@param CmdID			[In] Command id

@retval see FRAME_RESPONSE_STATUS_E

@author luoshuaitao
@date 2024/02/20
@note
**************************************************************************/
FRAME_RESPONSE_STATUS_E UpgradeFrameEncode(FRAME_HANDLE_T* pFrameHandle, unsigned char* pData, unsigned short DataLen,
							unsigned char Action , FRAME_REPLY_E ActionStatus, unsigned int CmdID)
{
	unsigned char HeaderSize, EndSize;
	unsigned int TempFrameNum = 0;
	FRAME_STRING_T* pFrameStr = &(pFrameHandle->FrameStr);

	if (NULL == pFrameHandle)
	{
		return FRAME_RESPONSE_ERROR;
	}
	if (DataLen > (UPGRADE_PROTOCOL_TRANSMIT_MAX_LEN - UPGRADE_PROTOCOL_FILED_LEN))
	{
		return FRAME_RESPONSE_TOO_LONG;
	}
	if (UpgradeFrameHeaderIsRight(&(pFrameHandle->FrameSend)) != 0)
	{
		return FRAME_RESPONSE_NOT_INIT;
	}

	HeaderSize = sizeof(pFrameHandle->FrameSend.Header) + sizeof(pFrameHandle->FrameSend.Version);
	EndSize = sizeof(pFrameHandle->FrameSend.CRC);

	/** Fill action */
	switch (Action)
	{
		case FRAME_ACTION_SEND_NO_REPLY: /**< If sending without response, handle it the same way as sending a frame */

		case FRAME_ACTION_SEND: /**< If it is sending a frame, user number+1, and all parameters are filled into the frame */
			pFrameHandle->FrameSend.FrameNumb++;
			pFrameHandle->FrameSend.Cmd.CmdID = CmdID;
			break;
		case FRAME_ACTION_REPLY: /**< If it is a reply frame, the user sequence number uses the sequence number of the reply frame,
										and the device address needs to be assigned separately */
			pFrameHandle->FrameSend.DstAddr = pFrameHandle->FrameRecv.SrcAddr;
			TempFrameNum = pFrameHandle->FrameSend.FrameNumb;
			pFrameHandle->FrameSend.FrameNumb = pFrameHandle->FrameRecv.FrameNumb;
			pFrameHandle->FrameSend.Cmd.CmdID = pFrameHandle->FrameRecv.Cmd.CmdID;
			pFrameHandle->FrameSend.Cmd.ActionStatus = ActionStatus;
			break;
		default:
			return FRAME_RESPONSE_ERROR;
	}
	pFrameHandle->FrameSend.Cmd.Action = Action;

	if(pData == NULL || DataLen == 0)
	{
		pFrameHandle->FrameSend.FrameData.DataCount = 0;
	}
	else if (pData != pFrameHandle->FrameSend.FrameData.pFrameData)
	{
		memcpy(pFrameHandle->FrameSend.FrameData.pFrameData, pData, DataLen);
		pFrameHandle->FrameSend.FrameData.DataCount = DataLen;
	}
	else
	{
		pFrameHandle->FrameSend.FrameData.DataCount = DataLen;
	}
	pFrameHandle->FrameSend.Length = pFrameHandle->FrameSend.FrameData.DataCount + UPGRADE_PROTOCOL_FILED_LEN;

	UpgradeFrame2String(&pFrameHandle->FrameSend, pFrameStr);

	pFrameHandle->FrameSend.CRC = CRC32Generate(pFrameStr->StrBuf + HeaderSize, pFrameStr->Len - HeaderSize - EndSize);
	UpgradeFrame2StringAddCRC(&pFrameHandle->FrameSend, pFrameStr);

	pFrameHandle->FrameSend.FrameNumb = (TempFrameNum) ? TempFrameNum : pFrameHandle->FrameSend.FrameNumb;

	return FRAME_RESPONSE_OK;
}

/**********************************************************************//**
@brief  Upgrade frame cache and decode

@param pFrameHandle		[In] Frame handle
@param pBuf				[In] pointer to frame data
@param Length			[In] Data length
@param pRemainLen		[In] Remain data length

@retval see FRAME_RESPONSE_STATUS_E

@author luoshuaitao
@date 2024/02/20
@note
**************************************************************************/
FRAME_RESPONSE_STATUS_E UpgradeFrameCacheAndDecode(FRAME_HANDLE_T* pFrameHandle, unsigned char* pBuf,
																	unsigned short Length, unsigned short* pRemainLen)
{
	FRAME_RESPONSE_STATUS_E Result = FRAME_RESPONSE_ERROR;

	if (NULL == pFrameHandle)
	{
		return FRAME_RESPONSE_ERROR;
	}

	if (pRemainLen != NULL)
	{
		*pRemainLen = 0;
	}

	unsigned char *pTempBuf = pBuf;
	unsigned short TempLen = Length;

	if ((pBuf == NULL || Length == 0) && pFrameHandle->RecvDataLen > UPGRADE_PROTOCOL_FRAME_HEADER_LEN)
	{
		pTempBuf = &(pFrameHandle->RecvData[UPGRADE_PROTOCOL_FRAME_HEADER_LEN]);
		TempLen = pFrameHandle->RecvDataLen - UPGRADE_PROTOCOL_FRAME_HEADER_LEN;
		pFrameHandle->HeaderFind = 0;
		pFrameHandle->RecvDataLen = 0;
	}

	for (unsigned short Index = 0; Index < TempLen; )
	{
		if (pFrameHandle->HeaderFind < UPGRADE_PROTOCOL_FRAME_HEADER_LEN)
		{
			if (pTempBuf[Index] == pFrameHandle->FrameSend.Header[pFrameHandle->HeaderFind])
			{
				pFrameHandle->RecvData[pFrameHandle->HeaderFind] = pTempBuf[Index];
				pFrameHandle->HeaderFind++;
				if (pFrameHandle->HeaderFind == UPGRADE_PROTOCOL_FRAME_HEADER_LEN)
				{
					pFrameHandle->RecvDataLen = pFrameHandle->HeaderFind;
				}
			}
			else if (pFrameHandle->HeaderFind > 0) 
			{
				pFrameHandle->HeaderFind = 0;
			}
		}
		else if (pFrameHandle->RecvDataLen < UPGRADE_PROTOCOL_FRAME_HEADER_LEN + sizeof(pFrameHandle->FrameSend.Length) +
				sizeof(pFrameHandle->FrameSend.Version) + sizeof(pFrameHandle->FrameSend.FrameNumb))
		{
			pFrameHandle->RecvData[pFrameHandle->RecvDataLen++] = pTempBuf[Index];
			if (pFrameHandle->RecvDataLen == UPGRADE_PROTOCOL_FRAME_HEADER_LEN + sizeof(pFrameHandle->FrameSend.Length) +
				sizeof(pFrameHandle->FrameSend.Version) + sizeof(pFrameHandle->FrameSend.FrameNumb))
			{
				pFrameHandle->FrameRecv.Length = UpgradeFrameCheckHead(pFrameHandle->RecvData);
				if (pFrameHandle->FrameRecv.Length > UPGRADE_PROTOCOL_LENGTH_MAX ||
					pFrameHandle->FrameRecv.Length < UPGRADE_PROTOCOL_FILED_LEN)
				{
					pFrameHandle->HeaderFind = 0;
					pFrameHandle->RecvDataLen = 0;
					pFrameHandle->FrameRecv.Length = 0;
					continue;
				}
			}
		}
		else
		{
			if ((TempLen - Index) >= (pFrameHandle->FrameRecv.Length - pFrameHandle->RecvDataLen))
			{
				memcpy((pFrameHandle->RecvData + pFrameHandle->RecvDataLen), (pTempBuf + Index), (pFrameHandle->FrameRecv.Length - pFrameHandle->RecvDataLen));

				Result = UpgradeFrameSearch(&pFrameHandle->FrameRecv, pFrameHandle->RecvData, pFrameHandle->FrameRecv.Length);
				if (Result != FRAME_RESPONSE_OK)
				{
					pFrameHandle->HeaderFind = 0;
					pFrameHandle->RecvDataLen = 0;
					continue;
				}
				else
				{
					if (pRemainLen != NULL)
					{
						*pRemainLen = TempLen - Index - (pFrameHandle->FrameRecv.Length - pFrameHandle->RecvDataLen);
					}
					pFrameHandle->HeaderFind = 0;
					pFrameHandle->RecvDataLen = 0;

				}
			}
			else
			{
				memcpy((pFrameHandle->RecvData + pFrameHandle->RecvDataLen), (pTempBuf + Index), (TempLen - Index));
				pFrameHandle->RecvDataLen += (TempLen - Index);
			}
			break;
		}

		Index++;
	}

	if (Result == FRAME_RESPONSE_ERROR)
	{
		Result = (pFrameHandle->HeaderFind) ? (FRAME_RESPONSE_FRAME_DECODING) : (FRAME_RESPONSE_ERROR);
	}

	return Result;
}

/**********************************************************************//**
@brief  Upgrade frame check head

@param pBuf			[In] Frame data buffer

@retval Frame data length

@author luoshuaitao
@date 2024/02/20
@note
**************************************************************************/
unsigned short UpgradeFrameCheckHead(unsigned char* pBuf)
{
	unsigned short RetVal = 0;

	if (pBuf == NULL)
	{
		return RetVal;
	}

	unsigned short Offset = 4;
	RetVal = ((unsigned short)pBuf[Offset] << 8) + (unsigned short)(pBuf[Offset + 1]);

	return RetVal;
}

/**********************************************************************//**
@brief  Upgrade frame clean cache

@param pFrameHandle			[In] Frame handle

@retval FRAME_RESPONSE_ERROR: Error
		FRAME_RESPONSE_OK: OK

@author luoshuaitao
@date 2024/02/20
@note
**************************************************************************/
FRAME_RESPONSE_STATUS_E UpgradeFrameCleanCache(FRAME_HANDLE_T* pFrameHandle)
{
	if (NULL == pFrameHandle)
	{
		return FRAME_RESPONSE_ERROR;
	}

	pFrameHandle->HeaderFind = 0;
	pFrameHandle->RecvDataLen = 0;
	pFrameHandle->FrameRecv.Length = 0;

	return FRAME_RESPONSE_OK;
}

