/**********************************************************************//**
			Firmware Upgrade

			Upgrade Module
*-
@file		UpgradeProtocol.h
@copyright	CHCNAV
@author		luoshuaitao
@date		2024/02/20
@brief

**************************************************************************/
#ifndef __UPGRADE_PROTOCOL_H__
#define __UPGRADE_PROTOCOL_H__

#if defined(__cplusplus)
	extern "C" {
#endif

#include <stdint.h>
#include "UpgradeProtocolCfgUser.h"

#define UPGRADE_PROTOCOL_VERSION					(0x00)

#define UPGRADE_PROTOCOL_FILED_LEN					(16U)	/**< The length of fields other than the frame data field */

#define UPGRADE_PROTOCOL_HEAD_LEN					(12U)	/**< Fixed frame length before frame data field */

#define UPGRADE_PROTOCOL_FRAME_HEADER_LEN		 	(2U)	/**< Frame start field length */


#if (!defined(UPGRADE_PROTOCOL_DATA_MAX_LEN) || \
	(defined(UPGRADE_PROTOCOL_DATA_MAX_LEN) && (UPGRADE_PROTOCOL_DATA_MAX_LEN < 1024U)))

	#define UPGRADE_PROTOCOL_DATA_MAX_LEN				(1024U)

#endif

#define UPGRADE_PROTOCOL_LENGTH_MAX			(UPGRADE_PROTOCOL_DATA_MAX_LEN + UPGRADE_PROTOCOL_FILED_LEN)

#if (!defined(UPGRADE_PROTOCOL_TRANSMIT_DATA_LEN))

	#define UPGRADE_PROTOCOL_TRANSMIT_DATA_LEN			(UPGRADE_PROTOCOL_DATA_MAX_LEN)

#endif

#define UPGRADE_PROTOCOL_TRANSMIT_MAX_LEN			(UPGRADE_PROTOCOL_FILED_LEN + UPGRADE_PROTOCOL_TRANSMIT_DATA_LEN)


#define UPGRADE_PROTOCOL_FRAME_HEADER				(0xD5A5U)

#define UPGRADE_PROTOCOL_FRAME_END

#ifndef UPGRADE_PROTOCOL_USE_CMDSHELL_COOPERATE

	#define UPGRADE_PROTOCOL_USE_CMDSHELL_COOPERATE          (0U)

#endif /**< UPGRADE_PROTOCOL_USE_CMDSHELL_COOPERATE */

typedef struct _FRAME_STRING_T
{
	unsigned char StrBuf[UPGRADE_PROTOCOL_TRANSMIT_MAX_LEN];	/**< String data buffer */
	unsigned short Len;											/**< String data length */
} FRAME_STRING_T;

typedef struct _PROTOCOL_FRAME_T
{
	unsigned char Header[2];				/**< Frame header */
	unsigned char Version;					/**< Protocol version */
	unsigned char FrameNumb;				/**< Frame number */
	unsigned short Length;					/**< Frame length */
	unsigned char SrcAddr;					/**< Frame source address */
	unsigned char DstAddr;					/**< Frame destination address */
	union
	{
		struct
		{
			unsigned int CmdID : 24;		/**< Command ID */
			unsigned char Reserved : 2;		/**< Reserved */
			unsigned char ActionStatus : 4;	/**< Command response status */
			unsigned char Action : 2;		/**< Command action */
		} Cmd;								/**< Command */
		unsigned int Field;
	};
	struct
	{
		unsigned char *pFrameData;			/**< Frame data pointer address */
		unsigned short DataCount;			/**< Data field, effective data length */
	} FrameData;							/**< Frame data */
	unsigned int CRC;						/**< CRC Verify Fields */
}PROTOCOL_FRAME_T;

typedef struct _FRAME_HANDLE_T
{
	PROTOCOL_FRAME_T FrameSend;				/**< Send Frame */
	PROTOCOL_FRAME_T FrameRecv;				/**< Received Frame */
	unsigned char RecvData[UPGRADE_PROTOCOL_LENGTH_MAX]; /**< Receive frame data */
	FRAME_STRING_T FrameStr;				/**< Convert frames into strings and store them in this variable */
	unsigned short (*Write)(char *, unsigned short);/**< Frame sending interface, which will be used for frame sending or response */
	unsigned short RecvDataLen;				/**< Cached effective frame data length */
	unsigned char HeaderFind;				/**< Find the length of the frame start field, where 2 represents found */
	unsigned int TransportDataLen;			/**< update transport data length */
} FRAME_HANDLE_T;

typedef enum _FRAME_ACTION_E
{
	FRAME_ACTION_SEND = 0,			/**< Command behavior - send, requires a response frame */
	FRAME_ACTION_REPLY,				/**< Command Behavior - Response, Response Sending Frame */
	FRAME_ACTION_SEND_NO_REPLY,	   /**< Command behavior - send without response */
} FRAME_ACTION_E;

typedef enum _FRAME_RESPONSE_STATUS_E
{
	FRAME_RESPONSE_OK = 0,						/**< response ok */
	FRAME_RESPONSE_ERROR,						/**< response error */
	FRAME_RESPONSE_TOO_LONG,					/**< The frame is too long, exceeding the UPGRADE-PROTOCOL-LENGTH-MAX limit */
	FRAME_RESPONSE_NOT_INIT,					/**< Frame object not initialized*/
	FRAME_RESPONSE_NOT_FOUND,					/**< No frame found */
	FRAME_RESPONSE_CRC_ERROR,					/**< CRC verification error */
	FRAME_RESPONSE_CMD_NOT_FOUND,				/**< Command not found */
	FRAME_RESPONSE_CMD_PARAM_MISSMATCH,		/**< Parameter mismatch */
	FRAME_RESPONSE_SEND_FAILED,					/**< Frame send failed */
	FRAME_RESPONSE_DEVICE_ID_NOT_MATCH,			/**< device id not match */
	FRAME_RESPONSE_START_FOUND,					/**< Frame start bit matching successful */
	FRAME_RESPONSE_FRAME_DECODING,				/**< Matching frame start field */
} FRAME_RESPONSE_STATUS_E;

typedef enum _FRAME_REPLY_E
{
	FRAME_REPLY_OK = 0,					/**< Reply-OK */
	FRAME_REPLY_ERROR,					/**< Reply-Error */
	FRAME_REPLY_CMD_NOT_FOUND,		 	/**< Reply-Not found command id */
	FRAME_REPLY_RETRY,					/**< Reply-Request resend */
	FRAME_REPLY_CMD_PARAM_ERROR,		/**< Reply-Command parameter error */
} FRAME_REPLY_E;

#define BSP_PROTO_BIN2INT32(x)			(((unsigned int)(*(unsigned char *)(x)) << 24) | \
										((unsigned int)(*(unsigned char *)(x + 1)) << 16) | \
										((unsigned int)(*(unsigned char *)(x + 2)) << 8) | \
										((unsigned int)(*(unsigned char *)(x + 3))));

#define BSP_PROTO_INT32TOBIN(x,val)		*(x) = (unsigned int)(*(unsigned int *)(&(val))) >> 24;\
										*(x + 1) = ((unsigned int)(*(unsigned int *)(&(val))) >> 16) & 0xFFU;\
										*(x + 2) = ((unsigned int)(*(unsigned int *)(&(val))) >> 8) & 0xFFU;\
										*(x + 3) = ((unsigned int)(*(unsigned int *)(&(val)))) & 0xFFU;

FRAME_RESPONSE_STATUS_E UpgradeFrameInit(FRAME_HANDLE_T* pFrameHandle, unsigned char SrcAddr, unsigned char DstAddr);
FRAME_RESPONSE_STATUS_E UpgradeFrameEncode(FRAME_HANDLE_T* pFrameHandle, unsigned char* pData, unsigned short DataLen,
							unsigned char Action , FRAME_REPLY_E ActionStatus, unsigned int CmdID);
FRAME_RESPONSE_STATUS_E UpgradeFrameCacheAndDecode(FRAME_HANDLE_T* pFrameHandle, unsigned char* pBuf,
																	unsigned short Length, unsigned short* pRemainLen);
unsigned short UpgradeFrameCheckHead(unsigned char* pBuf);
FRAME_RESPONSE_STATUS_E UpgradeFrameCleanCache(FRAME_HANDLE_T* pFrameHandle);

#if defined(__cplusplus)
}
#endif

#endif  /**< __UPGRADE_PROTOCOL_H__ */

