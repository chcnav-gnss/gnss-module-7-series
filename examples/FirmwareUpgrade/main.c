#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <termios.h>

#include "UpgradeM7xx.h"
#include "Version.h"
#include "UpgradeCfg.h"

static const unsigned int s_UartBaudRateTable[] = {9600, 19200, 115200, 460800, 921600, 1500000, 3000000};
static const unsigned int s_TransferSizeTable[] = {1024, 2048, 4096, 8192, 16384, 32768};

static int GetCmdArgs(int argc, char **argv, CMD_ARGS_T* pCmdArgs)
{
	int OptionCharacter;
	unsigned int Index;
	unsigned int TableSize;

	while ((OptionCharacter = getopt(argc, argv, "d:b:p:t:orcfuhv")) != -1)
	{
		switch (OptionCharacter)
		{
			case 'd':
				snprintf(pCmdArgs->Device, sizeof(pCmdArgs->Device), "%s", optarg);
				DEBUG_PRINTF("cmd args device:%s \n", pCmdArgs->Device);
			break;

			case 'b':
				pCmdArgs->Baudrate = atoi(optarg);
				TableSize = (sizeof(s_UartBaudRateTable)/sizeof(s_UartBaudRateTable[0]));
				for (Index = 0; Index < TableSize; Index++)
				{
					if (pCmdArgs->Baudrate == s_UartBaudRateTable[Index])
					{
						break;
					}
				}
				if (Index >= TableSize)
				{
					printf("Uart Baudrate value[%d] is error, baud rate in (9600, 115200, 460800, 921600, 1500000, 3000000)\r\n",pCmdArgs->Baudrate);
					return -1;
				}
				DEBUG_PRINTF("cmd args baudrate:%d \n", pCmdArgs->Baudrate);
			break;

			case 'p':
				snprintf(pCmdArgs->PkgPath, sizeof(pCmdArgs->PkgPath), "%s", optarg);
				DEBUG_PRINTF("cmd args package path:%s \n", pCmdArgs->PkgPath);
			break;

			case 't':
				pCmdArgs->TransferSize = atoi(optarg);
				TableSize = (sizeof(s_TransferSizeTable)/sizeof(s_TransferSizeTable[0]));
				for (Index = 0; Index < TableSize; Index++)
				{
					if (pCmdArgs->TransferSize == s_TransferSizeTable[Index])
					{
						break;
					}
				}
				if (Index >= TableSize)
				{
					printf("TransferSize value[%d] is error, transfer size in (4096, 8192, 16384, 32768)\r\n",pCmdArgs->TransferSize);
					return -1;
				}
				DEBUG_PRINTF("cmd args transfer size:%d \n", pCmdArgs->TransferSize);
			break;

			case 'o':
				pCmdArgs->DebugOutput = 1;
				SetDebugPrintf(1);
				DEBUG_PRINTF("cmd args debug output:%d \n", pCmdArgs->DebugOutput);
			break;

			case 'r':
				pCmdArgs->AutoReset = 1;
				DEBUG_PRINTF("# reset:\t %s\n", pCmdArgs->AutoReset == 1 ? "auto reset" : "no reset");
			break;
			case 'c':
				pCmdArgs->FastUpdate = 1;
				DEBUG_PRINTF("# FastUpdate:\t %s\n", pCmdArgs->FastUpdate == 1 ? "enable" : "disable");
			break;
			case 'f':
				pCmdArgs->ForceUpdate = 1;
				DEBUG_PRINTF("# ForceUpdate:\t %s\n", pCmdArgs->ForceUpdate == 1 ? "enable" : "disable");
			break;
			case 'u':
				pCmdArgs->RemoteDeviceUpdate = 1;
				DEBUG_PRINTF("# RemoteDeviceUpdate:\t %s\n", pCmdArgs->RemoteDeviceUpdate == 1 ? "enable" : "disable");
			break;
			case 'v':
				printf("Version:V%d.%d\n", MAJOR_VERSION, SUB_VERSION);
				exit(0);
			break;
			case 'h':
				printf("usage: ./update_m7xx -o -d device_path -b baudrate(down baudrate) -p package_path -t transfer_size -r -c -f -u\n");
				printf("example: ./update_m7xx -o -d /dev/ttySP1 -b 921600 -p Enc_M722_V2.1.31.1.pkg -t 2048 -r -c -f\n");
				printf("example: ./update_m7xx -o -d /dev/ttySP1 -b 921600 -p Radio_M4_B039.01.14.pkg -t 2048 -r -c -f -u\n");
				exit(0);
			break;

			case '?':
				printf("Invalid Option Character '%c' ! \n", (char)optopt);
			break;

			case ':':
				printf("Lack of option parameters! \n");
			break;
		}
	}

	return SUCCESS;
}

int main(int argc, char *argv[])
{
	int Result = 0;
	CMD_ARGS_T CmdArgs;

	memset(&CmdArgs, 0, sizeof(CMD_ARGS_T));

	CmdArgs.AutoReset = 0;
	CmdArgs.Baudrate = 115200;
	CmdArgs.TransferSize = 32768;
	CmdArgs.DebugOutput = 0;
	snprintf(CmdArgs.PkgPath, sizeof(CmdArgs.PkgPath), "/firmware/update_m7xx.pkg");
	snprintf(CmdArgs.Device, sizeof(CmdArgs.Device), "/dev/ttySP1");

	if (argc > 1)
	{
		if (GetCmdArgs(argc, argv, &CmdArgs) < 0)
		{
			printf("Input command args error!!!\n");
			return -1;
		}
		DEBUG_PRINTF("\n---------upgrade info----------\n");
		DEBUG_PRINTF("# reset:\t %s\n", CmdArgs.AutoReset == 1 ? "auto reset" : "no reset");
		DEBUG_PRINTF("# device:\t %s\n", CmdArgs.Device);
		DEBUG_PRINTF("# download baudrate:\t %d\n", CmdArgs.Baudrate);
		DEBUG_PRINTF("# package path:\t %s\n", CmdArgs.PkgPath);
		DEBUG_PRINTF("# download transfer size:\t %d\n", CmdArgs.TransferSize);
		DEBUG_PRINTF("# debug output:\t %d\n", CmdArgs.DebugOutput);
		DEBUG_PRINTF("-------------------------------\n");
	}

	Result = FitBaudrate(CmdArgs.Device, CmdArgs.Baudrate);
	if (Result < 0)
	{
		printf("Fit baudrate err!!!\n");
		return -1;
	}

	DEBUG_PRINTF("Upgrade start\n");

	return UpgradeM7xx(&CmdArgs);
}
