/**********************************************************************//**
			Uart Common

			Uart Module
*-
@file		UartCommon.c
@copyright	CHCNAV
@author		luoshuaitao
@date		2024/02/20
@brief

**************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>  
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "UartCommon.h"
#include <linux/serial.h>
#include <sys/ioctl.h>

/**********************************************************************//**
@brief  Uart open

@param pDevName		[In] Uart device name
@param Mode			[In] open mode

@retval <0 error, >=0 uart handle

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
int UartOpen(const char* pDevName, int Mode)
{
	int fd = 0;

	if (NULL == pDevName)
	{
		printf("Parameter error!\n");
		return -1;
	}

	fd = open(pDevName, O_RDWR | O_NOCTTY | O_NDELAY);
	if (-1 == fd)
	{
		printf("Can't open serial port!\n");
		return -1;
	}

	if (0 == Mode)
	{
		if (fcntl( fd, F_SETFL, O_NONBLOCK ) < 0)
		{
			printf("Uart: (mode = 0)Set mode error!\n");
			return -1;
		}
	}
	else if (fcntl( fd, F_SETFL, 0 ) < 0)
	{
		printf("Uart: (mode = 1)Set mode error!\n");
		return -1;
	}

	return fd;
}

/**********************************************************************//**
@brief  Uart option set

@param fd			[In] Uart handle
@param Speed		[In] Uart speed
@param Bits			[In] Uart Bits
@param Stop			[In] Uart Stop
@param Parity		[In] Uart Parity

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
int UartOptionSet(int fd, int Speed, unsigned char Bits, unsigned char Stop, char Parity)
{
	int Index = 0;
	struct termios Opt;
	int BaudTable[] = {
		B300, B600, B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B921600, B1500000, B3000000
	};

	int SpeedTable[] = {
		300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600, 1500000, 3000000
	};

	memset(&Opt, 0, sizeof(Opt));
	if (-1 == tcgetattr(fd, &Opt))
	{
		printf("Uart: tcgetattr set fail!\n");
		return -1;
	}

	cfmakeraw(&Opt);
	for (Index = 0; Index < sizeof(SpeedTable) / sizeof(int); Index++)
	{
		if(Speed == SpeedTable[Index])
		{
			cfsetispeed(&Opt, BaudTable[Index]);
			cfsetospeed(&Opt, BaudTable[Index]);
			if(-1 == tcsetattr(fd, TCSANOW, &Opt))
			{
				printf("Uart: tcsetattr set fail!\n");
				return -1;
			}
			break;
		}
	}

	if (-1 == tcgetattr(fd, &Opt))
	{
		printf("Uart: tcgetattr set fail!\n");
		return -1;
	}
	Opt.c_cflag |= CLOCAL | CREAD;
	Opt.c_cflag &= ~CRTSCTS;
	Opt.c_cflag &= ~CSIZE;

	switch (Bits)
	{
		case 7:
			Opt.c_cflag |= CS7;
			break;
		case 8:
			Opt.c_cflag |= CS8;
			break;
		default:
			printf("Uart: invalid paras!\n");
			return -1;
	}
	switch (Parity)
	{
		case 'n':
		case 'N':
			Opt.c_iflag &= ~INPCK;
			Opt.c_cflag &= ~PARENB;
			break;
		case 'o':
		case 'O':
			Opt.c_iflag |= INPCK;
			Opt.c_cflag |= PARENB;
			Opt.c_cflag |= PARODD;
			break;
		case 'e':
		case 'E':
			Opt.c_iflag |= INPCK;
			Opt.c_cflag |= PARENB;
			Opt.c_cflag &= ~PARODD;
			break;
		default:
			printf("Uart: invalid paras!\n");
			return -1;
	}

	switch (Stop)
	{
		case 1:
			Opt.c_cflag &= ~CSTOPB;
			break;
		case 2:
			Opt.c_cflag |= CSTOPB;
			break;
		default:
			printf("Uart: invalid paras!\n");
			return -1;
	}
	Opt.c_oflag &= ~OPOST;
	Opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	Opt.c_cc[VTIME] = 3;
	Opt.c_cc[VMIN] = 128;

	if (-1 == tcsetattr(fd, TCSANOW, &Opt))
	{
		printf("Uart: tcsetattr set fail!\n");
		return -1;
	}

	return 0;
}

/**********************************************************************//**
@brief  Uart read data

@param fd			[In] Uart handle
@param pBuff		[In] Pointer to data buffer
@param Size			[In] Data size

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
int UartRead(int fd, char* pBuff, int Size)
{
	if (NULL == pBuff)
	{
		printf("Parameter error: buff == NULL!\n");
		return -1;
	}

	return read(fd, pBuff, Size);
}

/**********************************************************************//**
@brief  Uart write data

@param fd			[In] Uart handle
@param pBuff		[In] Pointer to data buffer
@param Size			[In] Data size

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
int UartWrite(int fd, char* pBuff, int Size)
{
	int  iLen = 0;
	char *pCur = NULL;
	char *pLastPos = NULL;
	int  iTryCount = 1;

	if (NULL == pBuff)
	{
		printf("Parameter error: buff == NULL!\n");
		return -1;
	}

	iLen = 0;
	pCur = pBuff;
	pLastPos = pBuff + Size;

	while (pCur < pLastPos)
	{
		iLen = write(fd, pCur, (pLastPos - pCur));
		//printf("ilen: %d, strlen: %ld\n", iLen, pLastPos - pCur);
		if (iLen < 0)
		{
			if (errno != EAGAIN)
			{
				printf("com write err %d return!\n", errno);
				return -1;
			}
			else
			{
				iTryCount--;
				if (iTryCount <= 0)
				{
					printf("com write err return!\n");
					return -1;
				}
				usleep(1000);
			}
		}
		else
		{
			pCur += iLen;
			iTryCount = 1;
		}
	}

	return Size;
}

/**********************************************************************//**
@brief  Uart block mode

@param fd			[In] Uart handle
@param Mode			[In] Uart block mode

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
int UartBlockMode(int fd, int Mode)
{
	if (0 == Mode)
	{
		if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
		{
			printf("Uart: (mode = 0)Set mode error!\n");
			return -1;
		}
	}
	else
	{
		if (fcntl( fd, F_SETFL, 0 ) < 0)
		{
			printf("Uart: (mode = 1)Set mode error!\n");
			return -1;
		}
	}

	return 0;
}

/**********************************************************************//**
@brief  Uart close

@param fd			[In] Uart handle

@retval <0 error, =0 success

@author luoshuaitao
@date 2024/02/21
@note
**************************************************************************/
int UartClose(int fd)
{
	return close(fd);
}

