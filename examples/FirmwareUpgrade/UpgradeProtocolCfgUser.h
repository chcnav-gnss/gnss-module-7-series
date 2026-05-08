/**********************************************************************//**
			Firmware Upgrade

			Upgrade Module
*-
@file		UpgradeProtocolCfgUser.h
@copyright	CHCNAV
@author		luoshuaitao
@date		2024/02/20
@brief

**************************************************************************/
#ifndef __UPGRADE_PROTOCOL_CFG_USER_H__
#define __UPGRADE_PROTOCOL_CFG_USER_H__

#if defined(__cplusplus)
extern "C" {
#endif

#define UPGRADE_PROTOCOL_USE_CMDSHELL_COOPERATE		    (0U)	/**< Is transmission protocol and command engine collaboration used */

#define UPGRADE_PROTOCOL_DATA_MAX_LEN					(32 * 1024U)	/**< The length of the frame data field, must <= (32 * 1024) */

#define UPGRADE_PROTOCOL_TRANSMIT_DATA_LEN				(32 * 1024U)	/**< The maximum data field length of a frame sent by a frame */

typedef enum _UPGRADE_PROTOCOL_CMD_ID_E
{
	CHC_CMD_UPDATE_GET_DEVICE_INFO = 1,						/**< Get device information */
	CHC_CMD_UPDATE_REQUEST_REBOOT,							/**< request reboot */
	CHC_CMD_UPDATE_SEND_FIRMWARE,							/**< Send firmware */
	CHC_CMD_UPDATE_REQUEST_PREPARE_RECEIVE_FIRMWARE,		/**< Request prepare receive firmware */
	CHC_CMD_UPDATE_REQUEST_UPGRADE,							/**< Request upgrade */
	CHC_CMD_UPDATE_GET_UPGRADE_PROGRESS,					/**< Get upgrade progress */
	CHC_CMD_UPDATE_HANDSHAKE,								/**< Hand shake */
	CHC_CMD_RESERVED1,										/**< reserved */
	CHC_CMD_UPDATE_GET_UPDATE_STATUS = 9,					/**< Get update status */
	CHC_CMD_GET_DEVICE_STATUS,								/**< Get device status */
	CHC_CMD_GET_DEVICE_MAX_FRAME_LEN,						/**< Get the maximum frame data length supported by the device */
	CHC_CMD_SET_SEND_MAX_FRAME_LEN,							/**< Send max frame length */
	CHC_CMD_SET_FAST_UPDATE_MODE,							/**< Set fast update mode */
	CHC_CMD_RESERVED2,										/**< Reserved */
	CHC_CMD_SET_RADIO_UPDATE_TRANSFER_MODE,					/**< Set radio update transfer mode */
} UPGRADE_PROTOCOL_CMD_ID_E;

#if defined(__cplusplus)
}
#endif

#endif  /* __UPGRADE_PROTOCOL_CFG_USER_H__ */

