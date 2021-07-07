/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jun 3, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once


//TODO: make a enum class of these
enum CommandHandlerTypes {
	CTRL_CMD_SETUP                       = 0,
	CTRL_CMD_FACTORY_RESET               = 1,
	CTRL_CMD_STATE_GET                   = 2,
	CTRL_CMD_STATE_SET                   = 3,
	CTRL_CMD_GET_BOOTLOADER_VERSION      = 4,
	CTRL_CMD_GET_UICR_DATA               = 5,
	CTRL_CMD_SET_IBEACON_CONFIG_ID       = 6,
	CTRL_CMD_GET_MAC_ADDRESS             = 7,
	CTRL_CMD_GET_HARDWARE_VERSION        = 8,
	CTRL_CMD_GET_FIRMWARE_VERSION        = 9,


	CTRL_CMD_RESET                       = 10,
	CTRL_CMD_GOTO_DFU                    = 11,
	CTRL_CMD_NOP                         = 12,
	CTRL_CMD_DISCONNECT                  = 13,

	CTRL_CMD_SWITCH                      = 20,
	CTRL_CMD_MULTI_SWITCH                = 21,
	CTRL_CMD_PWM                         = 22,
	CTRL_CMD_RELAY                       = 23,

	CTRL_CMD_SET_TIME                    = 30,
	CTRL_CMD_INCREASE_TX                 = 31,
	CTRL_CMD_RESET_ERRORS                = 32,
	CTRL_CMD_MESH_COMMAND                = 33,
	CTRL_CMD_SET_SUN_TIME                = 34,
	CTRL_CMD_GET_TIME                    = 35,
	CTRL_CMD_RESET_MESH_TOPOLOGY         = 36,

	CTRL_CMD_ALLOW_DIMMING               = 40,
	CTRL_CMD_LOCK_SWITCH                 = 41,

	CTRL_CMD_UART_MSG                    = 50,
	CTRL_CMD_HUB_DATA                    = 51,

	CTRL_CMD_SAVE_BEHAVIOUR              = 60,
	CTRL_CMD_REPLACE_BEHAVIOUR           = 61,
	CTRL_CMD_REMOVE_BEHAVIOUR            = 62,
	CTRL_CMD_GET_BEHAVIOUR               = 63,
	CTRL_CMD_GET_BEHAVIOUR_INDICES       = 64,
	CTRL_CMD_GET_BEHAVIOUR_DEBUG         = 69,

	CTRL_CMD_REGISTER_TRACKED_DEVICE     = 70,
	CTRL_CMD_TRACKED_DEVICE_HEARTBEAT    = 71,
	CTRL_CMD_GET_PRESENCE                = 72,

	CTRL_CMD_GET_UPTIME                  = 80,
	CTRL_CMD_GET_ADC_RESTARTS            = 81,
	CTRL_CMD_GET_SWITCH_HISTORY          = 82,
	CTRL_CMD_GET_POWER_SAMPLES           = 83,
	CTLR_CMD_GET_SCHEDULER_MIN_FREE      = 84,
	CTRL_CMD_GET_RESET_REASON            = 85,
	CTRL_CMD_GET_GPREGRET                = 86,
	CTRL_CMD_GET_ADC_CHANNEL_SWAPS       = 87,
	CTRL_CMD_GET_RAM_STATS               = 88,

	CTRL_CMD_MICROAPP_GET_INFO           = 90,
	CTRL_CMD_MICROAPP_UPLOAD             = 91,
	CTRL_CMD_MICROAPP_VALIDATE           = 92,
	CTRL_CMD_MICROAPP_REMOVE             = 93,
	CTRL_CMD_MICROAPP_ENABLE             = 94,
	CTRL_CMD_MICROAPP_DISABLE            = 95,

	CTRL_CMD_CLEAN_FLASH                 = 100,

	CTRL_CMD_FILTER_UPLOAD               = 110,
	CTRL_CMD_FILTER_REMOVE               = 111,
	CTRL_CMD_FILTER_COMMIT               = 112,
	CTRL_CMD_FILTER_GET_SUMMARIES        = 113,

	CTRL_CMD_UNKNOWN                     = 0xFFFF
};

enum AdvCommandTypes {
	ADV_CMD_NOOP                       = 0,
	ADV_CMD_MULTI_SWITCH               = 1,
	ADV_CMD_SET_TIME                   = 2,
	ADV_CMD_SET_BEHAVIOUR_SETTINGS     = 3,
	ADV_CMD_UPDATE_TRACKED_DEVICE      = 4,
	ADV_CMD_UNKNOWN                    = 0xFF
};
