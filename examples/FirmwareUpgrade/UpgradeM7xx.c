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
			UpgradeFrameCleanCache(pFrameHandle);

			return PROTOCOL_FILTER_OK;
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

@retval <0:error =0:success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
static int SubPackageUpgradeProc(int UartFd, FILE* pFirmwareFile, FRAME_HANDLE_T* pFrameHandle, unsigned int FileSize, CMD_ARGS_T* pCmdArgs)
{
	unsigned int UpgradeStatus = GetUpgradeStatus();
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
		return -1;
	}

	if (UPGRADE_STATUS_IDLE != UpgradeStatus)
	{
		printf("Upgrade status is error: %d!\r\n", UpgradeStatus);
		return -1;
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
						SetUpgradeStatus(UPGRADE_STATUS_FAIL);
						printf("\r\nGet device information fail!\r\n");
						break;
					}
					FILE_INFO_T FileInfo = {0};
					ReadLen = ReadFirmwareData(pFirmwareFile, 0, (unsigned char*)&FileInfo, sizeof(FILE_INFO_T));
					if ((ReadLen < 0) || (ReadLen != sizeof(FILE_INFO_T)))
					{
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
							SetUpgradeStatus(UPGRADE_STATUS_FAIL);
							printf("\r\nRead firmware data fail!\r\n");
							break;
						}
						Result = UpgradeFrameDataEncode(pFrameHandle, CHC_CMD_UPDATE_SEND_FIRMWARE, ReadBuf, RequestSize);
						if (Result != 0)
						{
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
				printf("\r\nUpgrade fail!\r\n");
				SetUpgradeStatus(UPGRADE_STATUS_IDLE);
				return 0;
			case UPGRADE_STATUS_SUCCESS:
				PrintfUseTime(&StartTime, "UPGRADE_STATUS_SUCCESS:");

				PrintfUseTime(&StartTime1, "UPGRADE_STATUS_SUCCESS Total:");
				printf("\r\nUpgrade success!\r\n");
				SetUpgradeStatus(UPGRADE_STATUS_IDLE);
				return 0;
			default:
				SetUpgradeStatus(UPGRADE_STATUS_IDLE);
				return -1;
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

	return -1;
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

@retval <0:error =0:success

@author luoshuaitao
@date 2024/09/04
@note
**************************************************************************/
static int CheckUpdatePackage(char *pFirmwarePath, int* pFileSize)
{
	int FirmwareFileSize;
	unsigned char* pFileBuf = NULL;
	int ReadLen;

	if (!pFirmwarePath)
	{
		printf("Firmware path is null\r\n");
		return -1;
	}

	FILE* pFirmwareFile = fopen(pFirmwarePath, "rb");
	if (!pFirmwareFile)
	{
		printf("Failed to open firmware file\r\n");
		return -1;
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
		return -1;
	}

	ReadLen = fread(pFileBuf, sizeof(unsigned char), FirmwareFileSize, pFirmwareFile);
	if (ReadLen != FirmwareFileSize)
	{
		printf("Fail to read package body, please check buffsize[%d,%d]\r\n", ReadLen, FirmwareFileSize);
		free(pFileBuf);
		fclose(pFirmwareFile);
		return -1;
	}

	fclose(pFirmwareFile);

	if (CheckPackageM7xx(pFileBuf, FirmwareFileSize) < 0)
	{
		printf("Check firmware package file error\n");
		free(pFileBuf);
		return -1;
	}

	free(pFileBuf);

	*pFileSize = FirmwareFileSize;

	return 0;
}

/**********************************************************************//**
@brief  update m7xx firmware

@param pCmdArgs		[In] cmd args

@retval <0:error =0:success

@author luoshuaitao
@date 2024/09/04
@note
**************************************************************************/
int UpgradeM7xx(CMD_ARGS_T* pCmdArgs)
{
	int UartFd;
	int FirmwareFileSize = 0;
	FRAME_HANDLE_T FrameHandle;

	if (!pCmdArgs)
	{
		return -1;
	}

	CheckUpdatePackage(pCmdArgs->PkgPath, &FirmwareFileSize);

	UartFd = UartOpen(pCmdArgs->Device, 0);
	if (UartFd < 0)
	{
		printf("Uart open error!\r\n");
		return -1;
	}

	if(UartOptionSet(UartFd, pCmdArgs->Baudrate, 8, 1, 'n') < 0)
	{
		printf("Uart set option error!\r\n");
		close(UartFd);
		return -1;
	}

	FILE* pFirmwareFile = fopen(pCmdArgs->PkgPath, "rb");
	if (!pFirmwareFile)
	{
		printf("Failed to open firmware file\r\n");
		close(UartFd);
		return 1;
	}

	UpgradeFrameInit(&FrameHandle, 0, 0);

	if (SubPackageUpgradeProc(UartFd, pFirmwareFile, &FrameHandle, FirmwareFileSize, pCmdArgs) < 0)
	{
		printf("Sub package upgrade fail\r\n");
	}

	fclose(pFirmwareFile);
	UartClose(UartFd);

	return 0;
}

