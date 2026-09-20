#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <time.h>
#include <poll.h>


#include "md5.h"
#include "UpgradeM7xx.h"
#include "UpgradeEntry.h"
#include "UpgradeProtocol.h"
#include "UartCommon.h"
#include "UpgradeCfg.h"

#define UPDATE_OVERTIME		(25000U) /**< 25000ms */

typedef enum _PROTOCOL_FILTER_FRAM_ERR_E
{
	PROTOCOL_FILTER_ERR = -1,		/**< data filter error */
	PROTOCOL_WAIT_CONFIRM = 0,		/**< data not collected complete, need wait confirm */
	PROTOCOL_FILTER_OK,				/**< data analyze success */
} PROTOCOL_FILTER_FRAM_ERR_E;

static const int s_UartBaudRateTable[] = {115200, 3000000, 921600, 460800, 1500000, 9600, 19200};
static const unsigned int s_TransferSizeTable[] = {1024, 2048, 4096, 8192, 16384, 32768};

static unsigned int s_UpgradeStatus = UPGRADE_STATUS_IDLE;

int g_DebugPrintf = 0;

void SendUpgradeFrame(int UartFd, unsigned char* pData, unsigned int DataLen);

void SetDebugPrintf(int DebugPrintf)
{
	g_DebugPrintf = DebugPrintf;
}

static unsigned int GetUpgradeStatus(void)
{
	return s_UpgradeStatus;
}

static void SetUpgradeStatus(unsigned int Status)
{
	s_UpgradeStatus = Status;
}

#if (PRINTF_PROGRESS_ENABLE != 0)
/**********************************************************************//**
@brief  Print progress bar

@param Progress		[In] Progress

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
void PrintProgressBar(int Progress)
{
	int CompletedWidth = Progress * PROGRESS_BAR_WIDTH / 100;
	int RemainingWidth = PROGRESS_BAR_WIDTH - CompletedWidth;

	printf("[");
	for (int i = 0; i < CompletedWidth; i++)
	{
		printf("=");
	}
	for (int i = 0; i < RemainingWidth; i++)
	{
		printf(" ");
	}
	printf("] %d%%\r", Progress);
	fflush(stdout);
}

#endif

/**********************************************************************//**
@brief  Receive and filter an upgrade response frame

Read UART data and decode it through the frame cache. A complete frame is
reported as valid only when it is a reply whose command ID and frame number
match the current request in FrameSend. A reply matching the request immediately
preceding FrameSend triggers a resend of the cached current request without
re-encoding it. A reply using the current frame number but the preceding request
command ID also triggers this resend, which recovers from residual radio control
replies during the transition to transparent transfer mode. Other unmatched
frames are discarded so that they cannot be processed by the current upgrade
state.

@param UartFd			[In] UART file descriptor
@param pFrameHandle		[In] Frame handle containing the sent and received frames

@retval PROTOCOL_FILTER_OK		A response matching the current request was received
@retval PROTOCOL_WAIT_CONFIRM	No complete matching response is available yet
@retval PROTOCOL_FILTER_ERR		Frame decoding failed

@author luoshuaitao
@date 2026/09/15
@note FrameSend and FrameStr must retain the request that is currently awaiting
      a response. The resend path preserves its CmdID, FrameNum and payload.
**************************************************************************/
static int UpgradeFrameCacheAndFilter(int UartFd, FRAME_HANDLE_T* pFrameHandle)
{
	unsigned char ReadBuf[1024];
	int ReadLen;
	int Result;

	ReadLen = read(UartFd, ReadBuf, sizeof(ReadBuf));
	if (ReadLen > 0)
	{
		DEBUG_PRINTF("Read data len is %d\r\n", ReadLen);
		Result = UpgradeFrameCacheAndDecode(pFrameHandle, ReadBuf, ReadLen, NULL);
		if (Result == FRAME_RESPONSE_FRAME_DECODING) /**< frame decoding */
		{
			return PROTOCOL_WAIT_CONFIRM;
		}
		else if (Result == FRAME_RESPONSE_OK)
		{
			if ((pFrameHandle->FrameRecv.Cmd.Action == FRAME_ACTION_REPLY) &&
				(pFrameHandle->FrameRecv.Cmd.CmdID == pFrameHandle->FrameSend.Cmd.CmdID) &&
				(pFrameHandle->FrameRecv.FrameNumb == pFrameHandle->FrameSend.FrameNumb))
			{
				UpgradeFrameCleanCache(pFrameHandle);

				return PROTOCOL_FILTER_OK;
			}

			if ((pFrameHandle->FrameRecv.Cmd.Action == FRAME_ACTION_REPLY) &&
				(pFrameHandle->PreviousRequestValid == UPGRADE_REQUEST_HISTORY_VALID) &&
				(pFrameHandle->FrameRecv.Cmd.CmdID == pFrameHandle->PreviousRequestCmdID) &&
				(pFrameHandle->FrameRecv.FrameNumb == pFrameHandle->PreviousRequestFrameNumb) &&
				(pFrameHandle->FrameStr.Len > 0U))
			{
				DEBUG_PRINTF("Resend pending request: CmdID=0x%06X, FrameNum=%u, Len=%u; received previous reply(CmdID=0x%06X, FrameNum=%u)\r\n",
					(unsigned int)pFrameHandle->FrameSend.Cmd.CmdID,
					(unsigned int)pFrameHandle->FrameSend.FrameNumb,
					(unsigned int)pFrameHandle->FrameStr.Len,
					(unsigned int)pFrameHandle->FrameRecv.Cmd.CmdID,
					(unsigned int)pFrameHandle->FrameRecv.FrameNumb);
				UpgradeFrameCleanCache(pFrameHandle);
				SendUpgradeFrame(UartFd, pFrameHandle->FrameStr.StrBuf, pFrameHandle->FrameStr.Len);

				return PROTOCOL_WAIT_CONFIRM;
			}

			if ((pFrameHandle->FrameRecv.Cmd.Action == FRAME_ACTION_REPLY) &&
				(pFrameHandle->PreviousRequestValid == UPGRADE_REQUEST_HISTORY_VALID) &&
				(pFrameHandle->FrameRecv.FrameNumb == pFrameHandle->FrameSend.FrameNumb) &&
				(pFrameHandle->FrameRecv.Cmd.CmdID == pFrameHandle->PreviousRequestCmdID) &&
				(pFrameHandle->FrameRecv.Cmd.CmdID != pFrameHandle->FrameSend.Cmd.CmdID) &&
				(pFrameHandle->FrameStr.Len > 0U))
			{
				DEBUG_PRINTF("Resend pending request: CmdID=0x%06X, FrameNum=%u, Len=%u; received same-frame unmatched reply(CmdID=0x%06X)\r\n",
					(unsigned int)pFrameHandle->FrameSend.Cmd.CmdID,
					(unsigned int)pFrameHandle->FrameSend.FrameNumb,
					(unsigned int)pFrameHandle->FrameStr.Len,
					(unsigned int)pFrameHandle->FrameRecv.Cmd.CmdID);
				UpgradeFrameCleanCache(pFrameHandle);
				SendUpgradeFrame(UartFd, pFrameHandle->FrameStr.StrBuf, pFrameHandle->FrameStr.Len);

				return PROTOCOL_WAIT_CONFIRM;
			}

			DEBUG_PRINTF("Ignore unmatched response: Expected(CmdID=0x%06X, FrameNum=%u), Received(Action=%u, CmdID=0x%06X, FrameNum=%u)\r\n",
				(unsigned int)pFrameHandle->FrameSend.Cmd.CmdID,
				(unsigned int)pFrameHandle->FrameSend.FrameNumb,
				(unsigned int)pFrameHandle->FrameRecv.Cmd.Action,
				(unsigned int)pFrameHandle->FrameRecv.Cmd.CmdID,
				(unsigned int)pFrameHandle->FrameRecv.FrameNumb);
			UpgradeFrameCleanCache(pFrameHandle);

			return PROTOCOL_WAIT_CONFIRM;
		}
		else
		{
			UpgradeFrameCleanCache(pFrameHandle);

			return PROTOCOL_FILTER_ERR;
		}
	}

	return PROTOCOL_WAIT_CONFIRM;
}

/**********************************************************************//**
@brief  Send upgrade frame data

@param UartFd			[In] Uart handle
@param pData			[In] Pointer to data buffer
@param DataLen			[In] Data length

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
void SendUpgradeFrame(int UartFd, unsigned char* pData, unsigned int DataLen)
{
	write(UartFd, pData, DataLen);
	DEBUG_PRINTF("Send data len is %d\r\n", DataLen);
}

/**********************************************************************//**
@brief  Read firmware data

@param pFirmwareFile	[In] Firmware file handle
@param ReadAddr			[In] Read address
@param pData			[Out] Pointer to data buffer
@param DataLen			[In] Data length

@retval Read data length

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
int ReadFirmwareData(FILE* pFirmwareFile, unsigned int ReadAddr, unsigned char* pData, unsigned int DataLen)
{
	int ReadLen;

	fseek(pFirmwareFile, ReadAddr, SEEK_SET);
	ReadLen = fread(pData, sizeof(unsigned char), DataLen, pFirmwareFile);

	return ReadLen;
}

#if (PRINTF_USETIME_ENABLE != 0)
/**********************************************************************//**
@brief  different time value

@param pStart		[In] start time
@param pEnd			[In] end time

@retval different time value

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
long int DiffTimeVal(const struct timeval* pStart, const struct timeval* pEnd)
{
    double d;
    time_t s;
    suseconds_t u;

    s = pStart->tv_sec - pEnd->tv_sec;
    u = pStart->tv_usec - pEnd->tv_usec;

    d = s;
    d *= 1000000; /**< 1 sec = 10^6 us */
    d += u;

    return d;
}
#endif
/**********************************************************************//**
@brief  printf use time

@param pStartTime		[In] start time
@param pOutputChar		[In] output string
@param pData			[Out] Pointer to data buffer
@param DataLen			[In] Data length

@retval Read data length

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
static int PrintfUseTime(struct timeval* pStartTime, char* pOutputStr)
{
#if (PRINTF_USETIME_ENABLE != 0)
	struct timeval EndTime;

	gettimeofday(&EndTime, NULL);
	long int UseTime = DiffTimeVal(&EndTime, pStartTime);
	DEBUG_PRINTF("%s UseTime: %ld us\r\n", pOutputStr, UseTime);
	*pStartTime = EndTime;
#endif
	return 0;
}

/**********************************************************************//**
@brief  Sub package upgrade process

@param UartFd			[In] Uart handle
@param pFirmwareFile	[In] Firmware file handle
@param pFrameHandle		[In] Upgrade frame handle
@param FileSize			[In] Firmware file size
@param pCmdArgs			[In] cmd args

@retval UPGRADE_RESULT_OK:success, otherwise:upgrade procedure error

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
static int SubPackageUpgradeProc(int UartFd, FILE* pFirmwareFile, FRAME_HANDLE_T* pFrameHandle, unsigned int FileSize, CMD_ARGS_T* pCmdArgs)
{
	unsigned int UpgradeStatus = GetUpgradeStatus();
	int UpgradeResult = UPGRADE_RESULT_INTERNAL_FAILED;
	int FrameMaxLen = 0;
	unsigned int RequestAddr = 0, RequestSize = 0, RequestTotalSize = 0;
	unsigned int GetUpdateStatusCount = 0;
	int Result;
	int ReadLen;
	unsigned int TransportMaxLen = 0;
	unsigned int TimeCntr = 0;

	int Progress = -1;
	int NewProgress = 0;

	struct timeval StartTime = {0}, StartTime1 = {0};

	if ((UartFd < 0) || (!pFirmwareFile) || (!pFrameHandle) || (!pCmdArgs))
	{
		return UPGRADE_RESULT_INVALID_ARG;
	}

	if (UPGRADE_STATUS_IDLE != UpgradeStatus)
	{
		printf("Upgrade status is error: %d!\r\n", UpgradeStatus);
		return UPGRADE_RESULT_STATE_FAILED;
	}
#if (PRINTF_USETIME_ENABLE != 0)
	gettimeofday(&StartTime, NULL);
	StartTime1 = StartTime;
#endif

	while(1)
	{
		UpgradeStatus = GetUpgradeStatus();

		if (TimeCntr >= UPDATE_OVERTIME)
		{
			UpgradeResult = UPGRADE_RESULT_TIMEOUT;
			SetUpgradeStatus(UPGRADE_STATUS_FAIL);
			printf("\r\nUpgradeStatus: %d receiver over time!\r\n", UpgradeStatus);
		}

		switch(UpgradeStatus)
		{
			case UPGRADE_STATUS_IDLE:
				PrintfUseTime(&StartTime, "UPGRADE_STATUS_IDLE:");
				if (pCmdArgs->RemoteDeviceUpdate)
				{
					UPDATE_TRANSFER_MODE_INFO_T UpdateTransferMode = {RADIO_UPDATE_TRANSFER_MODE_OPEN , 0};
					UpgradeFrameDataEncode(pFrameHandle, CHC_CMD_SET_RADIO_UPDATE_TRANSFER_MODE, (unsigned char*)&UpdateTransferMode, sizeof(UPDATE_TRANSFER_MODE_INFO_T));
					SetUpgradeStatus(UPGRADE_STATUS_OPEN_RADIO_UPDATE_TRANSFER_MODE);
				}
				else
				{
					UpgradeFrameDataEncode(pFrameHandle, CHC_CMD_GET_DEVICE_MAX_FRAME_LEN, NULL, 0);
					SetUpgradeStatus(UPGRADE_STATUS_GET_DEVICE_MAX_FRAME_LEN);
				}
				SendUpgradeFrame(UartFd, pFrameHandle->FrameStr.StrBuf, pFrameHandle->FrameStr.Len);
				break;
			case UPGRADE_STATUS_OPEN_RADIO_UPDATE_TRANSFER_MODE:
				if (PROTOCOL_FILTER_OK == UpgradeFrameCacheAndFilter(UartFd, pFrameHandle))
				{
					PrintfUseTime(&StartTime, "UPGRADE_STATUS_OPEN_RADIO_UPDATE_TRANSFER_MODE:");
					if (SetRadioUpdateTransferModeReplyHandle(pFrameHandle) < 0)
					{
						UpgradeResult = UPGRADE_RESULT_PROTOCOL_FAILED;
						SetUpgradeStatus(UPGRADE_STATUS_FAIL);
						printf("\r\nOpen radio update transfer mode fail!\r\n");
						break;
					}

					UpgradeFrameDataEncode(pFrameHandle, CHC_CMD_GET_DEVICE_MAX_FRAME_LEN, NULL, 0);
					SendUpgradeFrame(UartFd, pFrameHandle->FrameStr.StrBuf, pFrameHandle->FrameStr.Len);
					SetUpgradeStatus(UPGRADE_STATUS_GET_DEVICE_MAX_FRAME_LEN);
					TimeCntr = 0;
				}
				break;
			case UPGRADE_STATUS_GET_DEVICE_MAX_FRAME_LEN:
				if (PROTOCOL_FILTER_OK == UpgradeFrameCacheAndFilter(UartFd, pFrameHandle))
				{
					PrintfUseTime(&StartTime, "UPGRADE_STATUS_GET_DEVICE_MAX_FRAME_LEN:");
					if (GetFrameMaxLenReplyHandle(pFrameHandle, &FrameMaxLen) < 0)
					{
						UpgradeResult = UPGRADE_RESULT_PROTOCOL_FAILED;
						SetUpgradeStatus(UPGRADE_STATUS_FAIL);
						printf("\r\nGet device max frame length fail!\r\n");
						break;
					}
					else
					{
						DEBUG_PRINTF("Module FrameMaxLen=0x%08X\r\n", FrameMaxLen);
					}

					unsigned int FrameDataLen = FrameMaxLen - UPGRADE_PROTOCOL_FILED_LEN;
					unsigned int Index;
					unsigned int TableSize = (sizeof(s_TransferSizeTable)/sizeof(s_TransferSizeTable[0]));
					for (Index = 0; Index < TableSize; Index++)
					{
						if (FrameDataLen == s_TransferSizeTable[Index])
						{
							break;
						}
					}
					if (Index >= TableSize)
					{
						UpgradeResult = UPGRADE_RESULT_PROTOCOL_FAILED;
						SetUpgradeStatus(UPGRADE_STATUS_FAIL);
						printf("Module FrameMaxLen error:%d\r\n", FrameDataLen);
						break;
					}

					if (pCmdArgs->TransferSize <= FrameDataLen)
					{
						TransportMaxLen = pCmdArgs->TransferSize + UPGRADE_PROTOCOL_FILED_LEN;
					}
					else
					{
						TransportMaxLen = FrameDataLen + UPGRADE_PROTOCOL_FILED_LEN;
					}

					UpgradeFrameDataEncode(pFrameHandle, CHC_CMD_SET_SEND_MAX_FRAME_LEN, (unsigned char*)&TransportMaxLen, sizeof(unsigned int));
					SendUpgradeFrame(UartFd, pFrameHandle->FrameStr.StrBuf, pFrameHandle->FrameStr.Len);
					SetUpgradeStatus(UPGRADE_STATUS_SEND_MAX_FRAME_LEN);
					TimeCntr = 0;
				}
				break;
			case UPGRADE_STATUS_SEND_MAX_FRAME_LEN:
				if (PROTOCOL_FILTER_OK == UpgradeFrameCacheAndFilter(UartFd, pFrameHandle))
				{
					PrintfUseTime(&StartTime, "UPGRADE_STATUS_SEND_MAX_FRAME_LEN:");
					if (SendFrameMaxLenReplyHandle(pFrameHandle) < 0)
					{
						UpgradeResult = UPGRADE_RESULT_PROTOCOL_FAILED;
						SetUpgradeStatus(UPGRADE_STATUS_FAIL);
						printf("\r\nSend device max frame length fail!\r\n");
						break;
					}
					UpgradeFrameDataEncode(pFrameHandle, CHC_CMD_UPDATE_GET_DEVICE_INFO, NULL, 0);
					SendUpgradeFrame(UartFd, pFrameHandle->FrameStr.StrBuf, pFrameHandle->FrameStr.Len);
					SetUpgradeStatus(UPGRADE_STATUS_GET_DEVICE_INFO);
					TimeCntr = 0;
				}
				break;
			case UPGRADE_STATUS_GET_DEVICE_INFO:
				if (PROTOCOL_FILTER_OK == UpgradeFrameCacheAndFilter(UartFd, pFrameHandle))
				{
					PrintfUseTime(&StartTime, "UPGRADE_STATUS_GET_DEVICE_INFO:");
					DEVICE_INFO_T DeviceInfo = {{0}};
					if (GetDeviceInfoReplyHandle(pFrameHandle, &DeviceInfo) < 0)
					{
						UpgradeResult = UPGRADE_RESULT_PROTOCOL_FAILED;
						SetUpgradeStatus(UPGRADE_STATUS_FAIL);
						printf("\r\nGet device information fail!\r\n");
						break;
					}
					FILE_INFO_T FileInfo = {0};
					ReadLen = ReadFirmwareData(pFirmwareFile, 0, (unsigned char*)&FileInfo, sizeof(FILE_INFO_T));
					if ((ReadLen < 0) || (ReadLen != sizeof(FILE_INFO_T)))
					{
						UpgradeResult = UPGRADE_RESULT_FILE_READ_FAILED;
						SetUpgradeStatus(UPGRADE_STATUS_FAIL);
						printf("\r\nRead file information fail!\r\n");
						break;
					}

					if (pCmdArgs->ForceUpdate)
					{
						FileInfo.Pad[0] = 0x51; /**< force update */
						FileInfo.Pad[1] = 0x5A;
					}
					RequestTotalSize += sizeof(FILE_INFO_T);
					UpgradeFrameDataEncode(pFrameHandle, CHC_CMD_UPDATE_REQUEST_UPGRADE, (unsigned char*)&FileInfo, sizeof(FILE_INFO_T));
					SendUpgradeFrame(UartFd, pFrameHandle->FrameStr.StrBuf, pFrameHandle->FrameStr.Len);
					SetUpgradeStatus(UPGRADE_STATUS_REQUEST_UPGRADE);
					TimeCntr = 0;
				}
				break;
			case UPGRADE_STATUS_REQUEST_UPGRADE:
				if (PROTOCOL_FILTER_OK == UpgradeFrameCacheAndFilter(UartFd, pFrameHandle))
				{
					PrintfUseTime(&StartTime, "UPGRADE_STATUS_REQUEST_UPGRADE:");
					if (RequestUpgradeReplyHandle(pFrameHandle) < 0)
					{
						UpgradeResult = UPGRADE_RESULT_PROTOCOL_FAILED;
						SetUpgradeStatus(UPGRADE_STATUS_FAIL);
						printf("\r\nRequest upgrade fail!\r\n");
						break;
					}

					if (pCmdArgs->FastUpdate)
					{
						UpgradeFrameDataEncode(pFrameHandle, CHC_CMD_SET_FAST_UPDATE_MODE, NULL, 0);
						SetUpgradeStatus(UPGRADE_STATUS_SET_FAST_UPDATE_MODE);
					}
					else
					{
						UpgradeFrameDataEncode(pFrameHandle, CHC_CMD_GET_DEVICE_STATUS, NULL, 0);
						SetUpgradeStatus(UPGRADE_STATUS_GET_DEVICE_STATUS);
					}
					SendUpgradeFrame(UartFd, pFrameHandle->FrameStr.StrBuf, pFrameHandle->FrameStr.Len);
					TimeCntr = 0;
				}
				break;
			case UPGRADE_STATUS_SET_FAST_UPDATE_MODE:
				if (PROTOCOL_FILTER_OK == UpgradeFrameCacheAndFilter(UartFd, pFrameHandle))
				{
					PrintfUseTime(&StartTime, "UPGRADE_STATUS_SET_FAST_UPDATE_MODE:");
					if (SetFastUpdateModeReplyHandle(pFrameHandle) < 0)
					{
						UpgradeResult = UPGRADE_RESULT_PROTOCOL_FAILED;
						SetUpgradeStatus(UPGRADE_STATUS_FAIL);
						printf("\r\nSet fast update mode fail!\r\n");
						break;
					}
					UpgradeFrameDataEncode(pFrameHandle, CHC_CMD_GET_DEVICE_STATUS, NULL, 0);
					SendUpgradeFrame(UartFd, pFrameHandle->FrameStr.StrBuf, pFrameHandle->FrameStr.Len);
					SetUpgradeStatus(UPGRADE_STATUS_GET_DEVICE_STATUS);
					TimeCntr = 0;
				}
				break;
			case UPGRADE_STATUS_GET_DEVICE_STATUS:
				if (PROTOCOL_FILTER_OK == UpgradeFrameCacheAndFilter(UartFd, pFrameHandle))
				{
					PrintfUseTime(&StartTime, "UPGRADE_STATUS_GET_DEVICE_STATUS:");
					if (GetDeviceStatusReplyHandle(pFrameHandle) < 0)
					{
						UpgradeResult = UPGRADE_RESULT_PROTOCOL_FAILED;
						SetUpgradeStatus(UPGRADE_STATUS_FAIL);
						printf("\r\nGet device status fail!\r\n");
						break;
					}
					UpgradeFrameDataEncode(pFrameHandle, CHC_CMD_UPDATE_SEND_FIRMWARE, NULL, 0);
					SendUpgradeFrame(UartFd, pFrameHandle->FrameStr.StrBuf, pFrameHandle->FrameStr.Len);
					SetUpgradeStatus(UPGRADE_STATUS_UPDATE_SEND_FIRMWARE);
					TimeCntr = 0;
				}
				break;
			case UPGRADE_STATUS_UPDATE_SEND_FIRMWARE:
				if (PROTOCOL_FILTER_OK == UpgradeFrameCacheAndFilter(UartFd, pFrameHandle))
				{
					PrintfUseTime(&StartTime, "UPGRADE_STATUS_UPDATE_SEND_FIRMWARE:");
					unsigned char ReadBuf[UPGRADE_PROTOCOL_DATA_MAX_LEN];

					if (SendSubPackageFirmwareReplyHandle(pFrameHandle, TransportMaxLen, &RequestAddr, &RequestSize) < 0)
					{
						UpgradeResult = UPGRADE_RESULT_PROTOCOL_FAILED;
						SetUpgradeStatus(UPGRADE_STATUS_FAIL);
						printf("\r\nSend firmware reply error, Please check firmware version!\r\n");
						break;
					}

					DEBUG_PRINTF("RequestAddr=0x%08X,RequestSize=%d\r\n",RequestAddr,RequestSize);

					if ((RequestAddr != 0) && (RequestSize != 0))
					{
						memset(ReadBuf, 0x00, sizeof(ReadBuf));
						ReadLen = ReadFirmwareData(pFirmwareFile, RequestAddr, ReadBuf, RequestSize);
						if ((ReadLen < 0) || (ReadLen != RequestSize))
						{
							UpgradeResult = UPGRADE_RESULT_FILE_READ_FAILED;
							SetUpgradeStatus(UPGRADE_STATUS_FAIL);
							printf("\r\nRead firmware data fail!\r\n");
							break;
						}
						Result = UpgradeFrameDataEncode(pFrameHandle, CHC_CMD_UPDATE_SEND_FIRMWARE, ReadBuf, RequestSize);
						if (Result != 0)
						{
							UpgradeResult = UPGRADE_RESULT_ENCODE_FAILED;
							SetUpgradeStatus(UPGRADE_STATUS_FAIL);
							printf("\r\nFrame Encode Error is %d\r\n",Result);
							break;
						}
						PrintfUseTime(&StartTime, "UpgradeFrameDataEncode:");
						RequestTotalSize += RequestSize;
						SendUpgradeFrame(UartFd, pFrameHandle->FrameStr.StrBuf, pFrameHandle->FrameStr.Len);
					}
					else /**< firmware send complete */
					{
						if (pCmdArgs->AutoReset == 1)
						{
							UpgradeFrameDataEncode(pFrameHandle, CHC_CMD_UPDATE_REQUEST_REBOOT, NULL, 0);
							SetUpgradeStatus(UPGRADE_STATUS_REQUEST_REBOOT);
							SendUpgradeFrame(UartFd, pFrameHandle->FrameStr.StrBuf, pFrameHandle->FrameStr.Len);
						}
						else
						{
							SetUpgradeStatus(UPGRADE_STATUS_SUCCESS);
						}
					}
					PrintfUseTime(&StartTime, "UPGRADE_STATUS_UPDATE_SEND_FIRMWARE:");
					TimeCntr = 0;
				}
				break;
			case UPGRADE_STATUS_REQUEST_REBOOT:
				if (PROTOCOL_FILTER_OK == UpgradeFrameCacheAndFilter(UartFd, pFrameHandle))
				{
					PrintfUseTime(&StartTime, "UPGRADE_STATUS_REQUEST_REBOOT:");
					if (RequestRebootReplyHandle(pFrameHandle) < 0)
					{
						UpgradeResult = UPGRADE_RESULT_PROTOCOL_FAILED;
						SetUpgradeStatus(UPGRADE_STATUS_FAIL);
						printf("\r\nGet update status fail!\r\n");
						break;
					}

					sleep(2);
					tcflush(UartFd, TCIOFLUSH);
				}

				if (pCmdArgs->RemoteDeviceUpdate)
				{
					UPDATE_TRANSFER_MODE_INFO_T UpdateTransferMode = {RADIO_UPDATE_TRANSFER_MODE_CLOSE , 0};
					UpgradeFrameDataEncode(pFrameHandle, CHC_CMD_SET_RADIO_UPDATE_TRANSFER_MODE, (unsigned char*)&UpdateTransferMode, sizeof(UPDATE_TRANSFER_MODE_INFO_T));
					SetUpgradeStatus(UPGRADE_STATUS_CLOSE_RADIO_UPDATE_TRANSFER_MODE);
					SendUpgradeFrame(UartFd, pFrameHandle->FrameStr.StrBuf, pFrameHandle->FrameStr.Len);
				}
				else
				{
					SetUpgradeStatus(UPGRADE_STATUS_SUCCESS);
				}

				break;
			case UPGRADE_STATUS_CLOSE_RADIO_UPDATE_TRANSFER_MODE:
				if (PROTOCOL_FILTER_OK == UpgradeFrameCacheAndFilter(UartFd, pFrameHandle))
				{
					PrintfUseTime(&StartTime, "UPGRADE_STATUS_CLOSE_RADIO_UPDATE_TRANSFER_MODE:");
					if (SetRadioUpdateTransferModeReplyHandle(pFrameHandle) < 0)
					{
						UpgradeResult = UPGRADE_RESULT_PROTOCOL_FAILED;
						SetUpgradeStatus(UPGRADE_STATUS_FAIL);
						printf("\r\nClose radio update transfer mode fail!\r\n");
						break;
					}

					SetUpgradeStatus(UPGRADE_STATUS_SUCCESS);
				}
				break;
			case UPGRADE_STATUS_GET_UPDATE_STATUS:
				if (PROTOCOL_FILTER_OK == UpgradeFrameCacheAndFilter(UartFd, pFrameHandle))
				{
					PrintfUseTime(&StartTime, "UPGRADE_STATUS_GET_UPDATE_STATUS:");
					if ((GetUpdateStatusReplyHandle(pFrameHandle, REPLY_UPGRADE_STATUS_IDLE) == 0) ||
						(GetUpdateStatusReplyHandle(pFrameHandle, REPLY_UPGRADE_STATUS_SUCCESS) == 0))
					{
						SetUpgradeStatus(UPGRADE_STATUS_SUCCESS);
						continue;
					}
					else
					{
						if (++GetUpdateStatusCount > 50) /**< wait 10s*/
						{
							UpgradeResult = UPGRADE_RESULT_TIMEOUT;
							SetUpgradeStatus(UPGRADE_STATUS_FAIL);
							printf("\r\nGet update status fail!\r\n");
							break;
						}
						else
						{
						}
					}
				}

				/** Send firmware complete, module will reset, request frame need to be sent on a scheduled basis */
				usleep(200000);
				UpgradeFrameDataEncode(pFrameHandle, CHC_CMD_UPDATE_GET_UPDATE_STATUS, NULL, 0);
				SendUpgradeFrame(UartFd, pFrameHandle->FrameStr.StrBuf, pFrameHandle->FrameStr.Len);
				break;
			case UPGRADE_STATUS_FAIL:
				PrintfUseTime(&StartTime, "UPGRADE_STATUS_FAIL:");
				printf("\r\nUpgrade fail! result=%d\r\n", UpgradeResult);
				SetUpgradeStatus(UPGRADE_STATUS_IDLE);
				if (UPGRADE_RESULT_OK == UpgradeResult)
				{
					return UPGRADE_RESULT_STATE_FAILED;
				}
				return UpgradeResult;
			case UPGRADE_STATUS_SUCCESS:
				PrintfUseTime(&StartTime, "UPGRADE_STATUS_SUCCESS:");

				PrintfUseTime(&StartTime1, "UPGRADE_STATUS_SUCCESS Total:");
				printf("\r\nUpgrade success!\r\n");
				SetUpgradeStatus(UPGRADE_STATUS_IDLE);
				return UPGRADE_RESULT_OK;
			default:
				SetUpgradeStatus(UPGRADE_STATUS_IDLE);
				return UPGRADE_RESULT_STATE_FAILED;
		}

		NewProgress = (int)((double)RequestTotalSize / FileSize * 100);

#if (PRINTF_PROGRESS_ENABLE != 0)
		if (NewProgress > Progress)
		{
			Progress = NewProgress;
			PrintProgressBar(Progress);
		}
#else
		if (NewProgress > Progress)
		{
			Progress = NewProgress;
			DEBUG_PRINTF("Progress: %d%%\r\n", Progress);
		}
#endif

		usleep(1000);

		TimeCntr++;
	}

	return UPGRADE_RESULT_INTERNAL_FAILED;
}

/**********************************************************************//**
@brief  Match Module

@param PortComFd		[In] uart fd

@retval <0:error =0:success

@author luoshuaitao
@date 2024/09/04
@note
**************************************************************************/
int MatchModule(int PortComFd)
{
	int Count = 0;
	int ReadSize = 0;
	char TempBuf[256] = {0};
	char ReadBuf[4096] = {0};
	char* pRespStr = ">RESPONSE,OK,\">OFFMSG*FF\"*05";
	int WaitCntr = 500; /**< wait 500 ms */

	int RespStrLen = strlen(pRespStr);

	tcflush(PortComFd, TCIOFLUSH);
	UartWrite(PortComFd, ">OFFMSG*FF\r\n", strlen(">OFFMSG*FF\r\n"));

	while(WaitCntr > 0)
	{
		Count = UartRead(PortComFd, TempBuf, sizeof(TempBuf));
		if (Count > 0)
		{
			if ((ReadSize + Count) < sizeof(ReadBuf))
			{
				memcpy(ReadBuf + ReadSize, TempBuf, Count);
				ReadSize += Count;

				if (ReadSize >= RespStrLen)
				{
					if (NULL != strstr(ReadBuf, pRespStr))
					{
						return 0;
					}
				}
			}
			else
			{
				return -1;
			}
		}

		usleep(1000);

		WaitCntr--;
	}
	
	return -1;
}

/**********************************************************************//**
@brief  fit baud rate

@param pDevName			[In] device name
@param Baudrate			[In] baud rate

@retval <0:error =0:success

@author luoshuaitao
@date 2024/09/04
@note
**************************************************************************/
int FitBaudrate(const char *pDevName, int Baudrate)
{
	int Index = 0;
	int PortComFd = 0;
	char TempCmdBuf[512] = {0};
	unsigned int UartBaudSize = sizeof(s_UartBaudRateTable) / sizeof(s_UartBaudRateTable[0]);

	DEBUG_PRINTF("FitBaudrate start\n");

	PortComFd = UartOpen(pDevName, 0); /**< Open the blocking mode, otherwise subsequent operations are meaningless */
	if (PortComFd < 0)
	{
		printf("Uart open failed, errno[ %d ] \n", errno);
		return -1;
	}

	DEBUG_PRINTF("Config com baudrate start\n");

	for (Index = 0; Index < UartBaudSize; Index++)
	{
		DEBUG_PRINTF("Switch baudrate %d\n", s_UartBaudRateTable[Index]);
		UartOptionSet(PortComFd, s_UartBaudRateTable[Index], 8, 1, 'n');

		if (0 == MatchModule(PortComFd))
		{
			break;
		}
	}

	if (Index >= UartBaudSize)
	{
		DEBUG_PRINTF("All baudrate connect fail\n");
		tcflush(PortComFd, TCIOFLUSH);
		UartClose(PortComFd);

		return -1;
	}

	snprintf(TempCmdBuf, sizeof(TempCmdBuf), ">CONFIGCOM,%d*FF\r\n", Baudrate);
	UartWrite(PortComFd, TempCmdBuf, strlen(TempCmdBuf));

	DEBUG_PRINTF("Config com baudrate end\n");

	usleep(500000);
	tcflush(PortComFd, TCIOFLUSH);
	UartClose(PortComFd);
	usleep(200000);

	PortComFd = UartOpen(pDevName, 0); /**< Open the blocking mode, otherwise subsequent operations are meaningless */
	if (PortComFd < 0)
	{
		printf("Uart open failed, errno[ %d ] \n", errno);
		return -1;
	}

	UartOptionSet(PortComFd, Baudrate, 8, 1, 'N');
	DEBUG_PRINTF("Set uart port baudrate:%d\n", Baudrate);
	usleep(200000);

	if (MatchModule(PortComFd) < 0)
	{
		DEBUG_PRINTF("Match module fail\n");
		tcflush(PortComFd, TCIOFLUSH);
		UartClose(PortComFd);

		return -1;
	}

	tcflush(PortComFd, TCIOFLUSH);
	UartClose(PortComFd);

	DEBUG_PRINTF("FitBaudrate OK\n");

	return 0;
}

/**********************************************************************//**
@brief  check package m7xx

@param pFileBuf			[In] firmware file buffer
@param FileSize			[In] firmware file size

@retval <0:error =0:success

@author luoshuaitao
@date 2024/09/04
@note
**************************************************************************/
static int CheckPackageM7xx(unsigned char* pFileBuf, int FileSize)
{
	FILE_INFO_T* pFileInfo = (FILE_INFO_T*)pFileBuf;
	PARTITION_INFO_T* pPartInfo = (PARTITION_INFO_T*)(pFileBuf + sizeof(FILE_INFO_T));
	FIRMWARE_INFO_T* pFirmwareInfo;
	unsigned int FWIndex, DevIndex;
	MD5_CTX_T MD5Ctx;
	unsigned char TempMD5[16];
	unsigned char FWNum;

	if (pFileInfo->FWNum > 0)
	{
		for (FWIndex = 0; FWIndex < pFileInfo->FWNum; FWIndex++)
		{
			pFirmwareInfo = (FIRMWARE_INFO_T*)(pFileBuf + pPartInfo->Addr[FWIndex]);

			MD5Start(&MD5Ctx);

			MD5Update(&MD5Ctx, pFileBuf + pPartInfo->Addr[FWIndex] + sizeof(FIRMWARE_INFO_T), pPartInfo->Size[FWIndex] - sizeof(FIRMWARE_INFO_T));

			MD5Finish(&MD5Ctx, TempMD5);

			if (0 != memcmp(pFirmwareInfo->MD5, TempMD5, sizeof(TempMD5)))
			{
				return -1;
			}
		}
	}

	FWNum = pFileInfo->FWNum;
	if (pFileInfo->DevNum > 0)
	{
		for (DevIndex = 0; DevIndex < pFileInfo->DevNum; DevIndex++)
		{
			pFirmwareInfo = (FIRMWARE_INFO_T*)(pFileBuf + pPartInfo->Addr[FWNum + DevIndex]);

			MD5Start(&MD5Ctx);

			MD5Update(&MD5Ctx, pFileBuf + pPartInfo->Addr[FWNum + DevIndex] + sizeof(FIRMWARE_INFO_T), pPartInfo->Size[FWNum + DevIndex] - sizeof(FIRMWARE_INFO_T));

			MD5Finish(&MD5Ctx, TempMD5);

			if (0 != memcmp(pFirmwareInfo->MD5, TempMD5, sizeof(TempMD5)))
			{
				return -1;
			}
		}
	}

	return 0;
}

/**********************************************************************//**
@brief  check update package

@param pFirmwarePath		[In] firmware path
@param pFileSize			[Out] firmware file size

@retval UPGRADE_RESULT_OK:success, otherwise:package check error

@author luoshuaitao
@date 2024/09/04
@note
**************************************************************************/
static int CheckUpdatePackage(char *pFirmwarePath, int* pFileSize)
{
	int FirmwareFileSize;
	unsigned char* pFileBuf = NULL;
	int ReadLen;

	if ((!pFirmwarePath) || (!pFileSize))
	{
		printf("Firmware path or file size is null\r\n");
		return UPGRADE_RESULT_INVALID_ARG;
	}

	FILE* pFirmwareFile = fopen(pFirmwarePath, "rb");
	if (!pFirmwareFile)
	{
		printf("Failed to open firmware file\r\n");
		return UPGRADE_RESULT_PACKAGE_CHECK_FAILED;
	}

	fseek(pFirmwareFile, 0, SEEK_END);
	FirmwareFileSize = (int)ftell(pFirmwareFile);
	DEBUG_PRINTF("Firmware file size is %d Byte\r\n", FirmwareFileSize);
	fseek(pFirmwareFile, 0, SEEK_SET);

	pFileBuf = (unsigned char*)malloc(FirmwareFileSize);
	if (pFileBuf == NULL)
	{
		printf("Malloc file size buffer error\n");
		fclose(pFirmwareFile);
		return UPGRADE_RESULT_PACKAGE_CHECK_FAILED;
	}

	ReadLen = fread(pFileBuf, sizeof(unsigned char), FirmwareFileSize, pFirmwareFile);
	if (ReadLen != FirmwareFileSize)
	{
		printf("Fail to read package body, please check buffsize[%d,%d]\r\n", ReadLen, FirmwareFileSize);
		free(pFileBuf);
		fclose(pFirmwareFile);
		return UPGRADE_RESULT_PACKAGE_CHECK_FAILED;
	}

	fclose(pFirmwareFile);

	if (CheckPackageM7xx(pFileBuf, FirmwareFileSize) < 0)
	{
		printf("Check firmware package file error\n");
		free(pFileBuf);
		return UPGRADE_RESULT_PACKAGE_CHECK_FAILED;
	}

	free(pFileBuf);

	*pFileSize = FirmwareFileSize;

	return UPGRADE_RESULT_OK;
}

/**********************************************************************//**
@brief  update m7xx firmware

@param pCmdArgs		[In] cmd args

@retval UPGRADE_RESULT_OK:success, otherwise:upgrade procedure error

@author luoshuaitao
@date 2024/09/04
@note
**************************************************************************/
int UpgradeM7xx(CMD_ARGS_T* pCmdArgs)
{
	int UartFd = -1;
	int FirmwareFileSize = 0;
	int UpgradeResult = UPGRADE_RESULT_INTERNAL_FAILED;
	FRAME_HANDLE_T FrameHandle;
	FILE* pFirmwareFile = NULL;

	if (!pCmdArgs)
	{
		return UPGRADE_RESULT_INVALID_ARG;
	}

	UpgradeResult = CheckUpdatePackage(pCmdArgs->PkgPath, &FirmwareFileSize);
	if (UPGRADE_RESULT_OK != UpgradeResult)
	{
		return UpgradeResult;
	}

	if (FitBaudrate(pCmdArgs->Device, pCmdArgs->Baudrate) < 0)
	{
		printf("Fit baudrate error!\r\n");
		return UPGRADE_RESULT_UART_CONFIG_FAILED;
	}

	UartFd = UartOpen(pCmdArgs->Device, 0);
	if (UartFd < 0)
	{
		printf("Uart open error!\r\n");
		return UPGRADE_RESULT_UART_OPEN_FAILED;
	}

	if(UartOptionSet(UartFd, pCmdArgs->Baudrate, 8, 1, 'n') < 0)
	{
		printf("Uart set option error!\r\n");
		UartClose(UartFd);
		return UPGRADE_RESULT_UART_CONFIG_FAILED;
	}

	pFirmwareFile = fopen(pCmdArgs->PkgPath, "rb");
	if (!pFirmwareFile)
	{
		printf("Failed to open firmware file\r\n");
		UartClose(UartFd);
		return UPGRADE_RESULT_FILE_OPEN_FAILED;
	}

	UpgradeFrameInit(&FrameHandle, 0, 0);

	UpgradeResult = SubPackageUpgradeProc(UartFd, pFirmwareFile, &FrameHandle, FirmwareFileSize, pCmdArgs);
	if (UPGRADE_RESULT_OK != UpgradeResult)
	{
		printf("Sub package upgrade fail, result=%d\r\n", UpgradeResult);
	}

	fclose(pFirmwareFile);
	UartClose(UartFd);

	return UpgradeResult;
}

