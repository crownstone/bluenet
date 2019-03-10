/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 23, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <cfg/cs_Config.h>
#include <cstdint>
#include <type_traits>
#include <cstddef> // For NULL
#include <drivers/cs_Serial.h>

typedef uint8_t* buffer_ptr_t;
typedef uint16_t buffer_size_t;
typedef uint16_t cs_ret_code_t;
typedef uint8_t  stone_id_t;
typedef uint16_t size16_t;

enum TypeBases {
	Configuration_Base = 0x000,
	State_Base         = 0x080,
	General_Base       = 0x100,
};

/** Cast to underlying type.
 *
 * This can be used in the following way.
 *
 *   CS_TYPE type = CONFIG_TX_POWER;
 *   uint8_t raw_value = to_underlying_type(type);
 *
 * This should be used very rarely. Use the CS_TYPE wherever possible.
 */
template <typename T>
constexpr auto to_underlying_type(T e) noexcept
-> std::enable_if_t<std::is_enum<T>::value, std::underlying_type_t<T>> {
	return static_cast<std::underlying_type_t<T>>(e);
}

/** State variable types
 * Also used as event types
 * Use in the characteristic to read and write state variables in <CommonService>.
 */
enum class CS_TYPE: uint16_t {
	CONFIG_DO_NOT_USE                       = Configuration_Base,   // Record keys should be in the range 0x0001 - 0xBFFF. The value 0x0000 is reserved by the system. The values from 0xC000 to 0xFFFF are reserved for use by the Peer Manager module and can only be used in applications that do not include Peer Manager.
//	CONFIG_DEVICE_TYPE                      = 1,      //  0x01
//	CONFIG_ROOM                             = 2,      //  0x02
//	CONFIG_FLOOR                            = 3,      //  0x03
//	CONFIG_NEARBY_TIMEOUT                   = 4,      //  0x04
	CONFIG_PWM_PERIOD                       = 5,      //  0x05
	CONFIG_IBEACON_MAJOR                    = 6,      //  0x06
	CONFIG_IBEACON_MINOR                    = 7,      //  0x07
	CONFIG_IBEACON_UUID                     = 8,      //  0x08
	CONFIG_IBEACON_TXPOWER                  = 9,      //  0x09
//	CONFIG_WIFI_SETTINGS                    = 10,     //  0x0A
	CONFIG_TX_POWER                         = 11,     //  0x0B
	CONFIG_ADV_INTERVAL                     = 12,     //  0x0C
//	CONFIG_PASSKEY                          = 13,     //  0x0D
//	CONFIG_MIN_ENV_TEMP                     = 14,     //  0x0E
//	CONFIG_MAX_ENV_TEMP                     = 15,     //  0x0F
	CONFIG_SCAN_DURATION                    = 16,     //  0x10   // Deprecate
//	CONFIG_SCAN_SEND_DELAY                  = 17,     //  0x11
	CONFIG_SCAN_BREAK_DURATION              = 18,     //  0x12   // Deprecate
	CONFIG_BOOT_DELAY                       = 19,     //  0x13
	CONFIG_MAX_CHIP_TEMP                    = 20,     //  0x14
//	CONFIG_SCAN_FILTER                      = 21,     //  0x15
//	CONFIG_SCAN_FILTER_SEND_FRACTION        = 22,     //  0x16
	CONFIG_CURRENT_LIMIT                    = 23,     //  0x17   // Not implemented yet, but something we want in the future.
	CONFIG_MESH_ENABLED                     = 24,     //  0x18
	CONFIG_ENCRYPTION_ENABLED               = 25,     //  0x19
	CONFIG_IBEACON_ENABLED                  = 26,     //  0x1A
	CONFIG_SCANNER_ENABLED                  = 27,     //  0x1B
//	CONFIG_CONT_POWER_SAMPLER_ENABLED       = 28,     //  0x1C
//	CONFIG_TRACKER_ENABLED                  = 29,     //  0x1D
//	CONFIG_ADC_BURST_SAMPLE_RATE            = 30,     //  0x1E
//	CONFIG_POWER_SAMPLE_BURST_INTERVAL      = 31,     //  0x1F
//	CONFIG_POWER_SAMPLE_CONT_INTERVAL       = 32,     //  0x20
//	CONFIG_ADC_CONT_SAMPLE_RATE             = 33,     //  0x1E
	CONFIG_CROWNSTONE_ID                    = 34,     //  0x22
	CONFIG_KEY_ADMIN                        = 35,     //  0x23
	CONFIG_KEY_MEMBER                       = 36,     //  0x24
	CONFIG_KEY_GUEST                        = 37,     //  0x25
	CONFIG_DEFAULT_ON                       = 38,     //  0x26
	CONFIG_SCAN_INTERVAL                    = 39,     //  0x27
	CONFIG_SCAN_WINDOW                      = 40,     //  0x28
	CONFIG_RELAY_HIGH_DURATION              = 41,     //  0x29
	CONFIG_LOW_TX_POWER                     = 42,     //  0x2A
	CONFIG_VOLTAGE_MULTIPLIER               = 43,     //  0x2B
	CONFIG_CURRENT_MULTIPLIER               = 44,     //  0x2C
	CONFIG_VOLTAGE_ADC_ZERO                 = 45,     //  0x2D
	CONFIG_CURRENT_ADC_ZERO                 = 46,     //  0x2E
	CONFIG_POWER_ZERO                       = 47,     //  0x2F
//	CONFIG_POWER_ZERO_AVG_WINDOW            = 48,     //  0x30
//	CONFIG_MESH_ACCESS_ADDRESS              = 49,     //  0x31
	CONFIG_SOFT_FUSE_CURRENT_THRESHOLD      = 50,     //  0x32
	CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM  = 51,     //  0x33
	CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP    = 52,     //  0x34
	CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN  = 53,     //  0x35
	CONFIG_PWM_ALLOWED                      = 54,     //  0x36
	CONFIG_SWITCH_LOCKED                    = 55,     //  0x37
	CONFIG_SWITCHCRAFT_ENABLED              = 56,     //  0x38
	CONFIG_SWITCHCRAFT_THRESHOLD            = 57,     //  0x39
//	CONFIG_MESH_CHANNEL                     = 58,     //  0x3A
	CONFIG_UART_ENABLED                     = 59,     //  0x3B
	CONFIG_NAME                             = 60,     //  0x3C

	STATE_RESET_COUNTER = State_Base,                 //  0x80 - 128
	STATE_SWITCH_STATE,                               //  0x81 - 129
	STATE_ACCUMULATED_ENERGY,                         //  0x82 - 130
	STATE_POWER_USAGE,                                //  0x83 - 131
//	STATE_TRACKED_DEVICES,                            //  0x84 - 132
	STATE_SCHEDULE,                                   //  0x85 - 133
	STATE_OPERATION_MODE,                             //  0x86 - 134
	STATE_TEMPERATURE,                                //  0x87 - 135
	STATE_TIME,                                       //  0x88 - 136
	STATE_FACTORY_RESET,                              //  0x89 - 137
//	STATE_LEARNED_SWITCHES,                           //  0x8A - 138
	STATE_ERRORS,                                     //  0x8B - 139 // TODO: deprecate
	STATE_ERROR_OVER_CURRENT,                         //  0x8C - 140
	STATE_ERROR_OVER_CURRENT_DIMMER,                  //  0x8D - 141
	STATE_ERROR_CHIP_TEMP,                            //  0x8E - 142
	STATE_ERROR_DIMMER_TEMP,                          //  0x8F - 143
//	STATE_IGNORE_BITMASK,                             //  0x90 - 144
//	STATE_IGNORE_ALL,                                 //  0x91 - 145
//	STATE_IGNORE_LOCATION,                            //  0x92 - 146
	STATE_ERROR_DIMMER_ON_FAILURE,                    //  0x93 - 147
	STATE_ERROR_DIMMER_OFF_FAILURE,                   //  0x94 - 148

	/*
	 * Internal commands and events.
	 * These don't need a specific order, as they're not exposed.
	 * Start at General_Base
	 */
	CMD_SWITCH_OFF = General_Base,                    // Sent to turn switch off.
	CMD_SWITCH_ON,                                    // Sent to turn switch on.
	CMD_SWITCH_TOGGLE,                                // Sent to toggle switch.
	EVT_ADVERTISEMENT_UPDATED,                        // Sent when advertisement was updated. TODO: advertisement data as payload?
	EVT_SCAN_STARTED,                                 // Sent when scanner started scanning.
	EVT_SCAN_STOPPED,                                 // Sent when scanner stopped scanning.
//	EVT_SCANNED_DEVICES,
	EVT_DEVICE_SCANNED,                               // Sent when a device was scanned. -- Payload is scanned_device_t.
	EVT_POWER_SAMPLES_START,                          // Sent when the power samples buffer (for characteristic) is being filled with new data.
	EVT_POWER_SAMPLES_END,                            // Sent when the power samples buffer (for characteristic) has been filled with new data.
	EVT_CURRENT_USAGE_ABOVE_THRESHOLD_DIMMER,         // Sent when current usage goes over the dimmer threshold, while dimmer is on.
	EVT_CURRENT_USAGE_ABOVE_THRESHOLD,                // Sent when current usage goes over the threshold.
	EVT_DIMMER_ON_FAILURE_DETECTED,                   // Sent when dimmer leaks current, while it's off.
	EVT_DIMMER_OFF_FAILURE_DETECTED,                  // Sent when dimmer blocks current, while it's on.
	EVT_MESH_TIME,                                    // Sent when the mesh received the current time
	EVT_SCHEDULE_ENTRIES_UPDATED,      // TODO: deprecate and use STATE event for this.                // Sent when schedule entries were changed. Payload is schedule_list_t.
//	EVT_BLE_EVENT,
	EVT_BLE_CONNECT,                                  // Sent when device connected.
	EVT_BLE_DISCONNECT,                               // Sent when device disconnected.
//	EVT_STATE_NOTIFICATION,            // Deprecated  // Sent when a state was updated.
	EVT_BROWNOUT_IMPENDING,                           // Sent when brownout is impending (low chip supply voltage)
	EVT_SESSION_NONCE_SET,                            // Sent when a session nonce is generated. -- Payload is the session nonce.
	EVT_KEEP_ALIVE,                                   // Sent when a keep alive has been received. -- Payload is keep_alive_state_message_payload_t.
	EVT_DIMMER_FORCED_OFF,                            // Sent when dimmer was forced off.
	EVT_SWITCH_FORCED_OFF,                            // Sent when switch (relay and dimmer) was forced off.
	EVT_RELAY_FORCED_ON,                              // Sent when relay was forced on.
	EVT_CHIP_TEMP_ABOVE_THRESHOLD,                    // Sent when chip temperature is above threshold.
	EVT_CHIP_TEMP_OK,                                 // Sent when chip temperature is ok again.
	EVT_DIMMER_TEMP_ABOVE_THRESHOLD,                  // Sent when dimmer temperature is above threshold.
	EVT_DIMMER_TEMP_OK,                               // Sent when dimmer temperature is ok again.
//	EVT_EXTERNAL_STATE_MSG_CHAN_0,     // Deprecated
//	EVT_EXTERNAL_STATE_MSG_CHAN_1,     // Deprecated
	EVT_TIME_SET,                                     // Sent when the time is set or changed. TODO: time as payload?
	EVT_DIMMER_POWERED,                               // Sent when dimmer is powered, and ready to be used.
	EVT_DIMMING_ALLOWED,                              // Sent when allow dimming is changed. -- Payload is bool.
	EVT_SWITCH_LOCKED,                                // Sent when switch locked flag is set. -- Payload is bool.
	EVT_STORAGE_WRITE_DONE,                           // Sent when an item has been written to storage. -- Payload is CS_TYPE, the type that was written.
	EVT_SETUP_DONE,                                   // Sent when setup was done (and settings are stored).
//	EVT_DO_RESET_DELAYED,                             // Sent to perform a reset in a few seconds.
	EVT_SWITCHCRAFT_ENABLED,                          // Sent when switchcraft flag is set. -- Payload is boolean.
//	EVT_STORAGE_WRITE,                                // Sent when an item is going to be written to storage.
//	EVT_STORAGE_ERASE,                                // Sent when a flash page is going to be erased.
	EVT_ADC_RESTARTED,                                // Sent when ADC has been restarted.
//	CMD_SET_LOG_LEVEL,
	CMD_ENABLE_LOG_POWER,                             // Sent to enable/disable power calculations logging. -- Payload is bool.
	CMD_ENABLE_LOG_CURRENT,                           // Sent to enable/disable current samples logging. -- Payload is bool.
	CMD_ENABLE_LOG_VOLTAGE,                           // Sent to enable/disable voltage samples logging. -- Payload is bool.
	CMD_ENABLE_LOG_FILTERED_CURRENT,                  // Sent to enable/disable filtered current samples logging. -- Payload is bool.
	CMD_RESET_DELAYED,                                // Sent to reboot. -- Payload is reset_delayed_t.
	CMD_ENABLE_ADVERTISEMENT,                         // Sent to enable/disable advertising. -- Payload is bool.
	CMD_ENABLE_MESH,                                  // Sent to enable/disable mesh. -- Payload is bool.
	CMD_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN,         // Sent to toggle ADC voltage pin. TODO: pin as payload?
	CMD_ENABLE_ADC_DIFFERENTIAL_CURRENT,              // Sent to toggle differential mode on current pin. -- Payload is bool.
	CMD_ENABLE_ADC_DIFFERENTIAL_VOLTAGE,              // Sent to toggle differential mode on voltage pin. -- Payload is bool.
	CMD_INC_VOLTAGE_RANGE,                            // Sent to increase voltage range.
	CMD_DEC_VOLTAGE_RANGE,                            // Sent to decrease voltage range.
	CMD_INC_CURRENT_RANGE,                            // Sent to increase current range.
	CMD_DEC_CURRENT_RANGE,                            // Sent to decrease current range.
	CMD_CONTROL_CMD,                                  // Sent to handle a control command. -- Payload is stream_header_t
	CMD_SET_OPERATION_MODE,                           // Sent to switch operation mode. -- Payload is OperationMode.
};

/*---------------------------------------------------------------------------------------------------------------------
 *
 *                                               Custom structs
 *
 *-------------------------------------------------------------------------------------------------------------------*/

struct state_vars_notifaction {
	uint8_t type;
	uint8_t* data;
	uint16_t dataLength;
};

union state_errors_t {
	struct __attribute__((packed)) {
		uint8_t overCurrent: 1;
		uint8_t overCurrentPwm: 1;
		uint8_t chipTemp: 1;
		uint8_t pwmTemp: 1;
		uint8_t dimmerOn: 1;
		uint8_t dimmerOff: 1;
		uint32_t reserved: 26;
	} errors;
	uint32_t asInt;
	state_errors_t(): asInt(0) {}
	state_errors_t(uint32_t val): asInt(val) {}
};

union __attribute__((__packed__)) __attribute__((__aligned__(4))) switch_state_t {
	struct __attribute__((packed)) {
		uint8_t dimmer : 7;
		uint8_t relay : 1;
	} state;
	uint8_t asInt;
	switch_state_t(): asInt(0) {}
	switch_state_t(uint8_t val) {
//		state.relay = val & 0x80;
//		state.dimmer = val & 0x7F;
		asInt = val;
	}
};

#define SESSION_NONCE_LENGTH 5
struct __attribute__((packed)) session_nonce_t {
	uint8_t data[SESSION_NONCE_LENGTH];
};

// TODO: check if this length is similar to BLE_GAP_ADDR_LEN
#define MAC_ADDRESS_LEN 6
#define ADVERTISEMENT_DATA_MAX_SIZE 31

struct __attribute__((packed)) scanned_device_t {
	int8_t rssi;
	uint8_t address[MAC_ADDRESS_LEN];
	uint8_t channel;
	uint8_t dataSize;
//	uint8_t data[ADVERTISEMENT_DATA_MAX_SIZE];
	uint8_t *data;
	// See ble_gap_evt_adv_report_t
	// More possibilities: addressType, connectable, isScanResponse, directed, scannable, extended advertisements, etc.
};


// TODO: these definitions (also the structs) should be moved to somewhere else.
#define DAILY_REPEAT_BIT_SUNDAYS    0
#define DAILY_REPEAT_BIT_MONDAYS    1
#define DAILY_REPEAT_BIT_TUESDAYS   2
#define DAILY_REPEAT_BIT_WEDNESDAYS 3
#define DAILY_REPEAT_BIT_THURSDAYS  4
#define DAILY_REPEAT_BIT_FRIDAYS    5
#define DAILY_REPEAT_BIT_SATURDAYS  6
#define DAILY_REPEAT_BIT_ALL_DAYS   7

#define SCHEDULE_TIME_TYPE_REPEAT      0
#define SCHEDULE_TIME_TYPE_DAILY       1
#define SCHEDULE_TIME_TYPE_ONCE        2

#define SCHEDULE_ACTION_TYPE_PWM       0
#define SCHEDULE_ACTION_TYPE_FADE      1
#define SCHEDULE_ACTION_TYPE_TOGGLE    2

struct __attribute__((__packed__)) schedule_time_daily_t {
	//! Only perform action on certain days these days of the week. Bitmask, see DAILY_REPEAT_*.
	//! Check against (1 << current_day_of_week)
	//! If (dayOfWeek & DAILY_REPEAT_ALL_DAYS), then the other bits are ignored.
	uint8_t dayOfWeekBitmask;
	uint8_t reserved;
};

struct __attribute__((__packed__)) schedule_time_repeat_t {
	uint16_t repeatTime; //! Repeat every X minutes. 0 is not allowed.
};

struct __attribute__((__packed__)) schedule_action_pwm_t {
	uint8_t pwm;
	uint8_t reserved[2];
};

struct __attribute__((__packed__)) schedule_action_fade_t {
	uint8_t pwmEnd;
	uint16_t fadeDuration; //! Number of seconds it takes to get to pwmEnd.
};

struct __attribute__((__packed__)) schedule_entry_t {
	uint8_t reserved;

	// Combined time and action type.
	// Defined as SCHEDULE_TIME_TYPE_.. + (SCHEDULE_ACTION_TYPE_.. << 4)
	uint8_t type;

	uint8_t overrideMask; //! What to override. Bitmask, see SCHEDULE_OVERRIDE_*

	uint32_t nextTimestamp;
	union {
		schedule_time_repeat_t repeat;
		schedule_time_daily_t daily;
	};
	union {
		schedule_action_pwm_t pwm;
		schedule_action_fade_t fade;
	};
};

// Size: 1 + 12*MAX_SCHEDULE_ENTRIES
struct __attribute__((__packed__)) schedule_list_t {
	uint8_t size;
	schedule_entry_t list[MAX_SCHEDULE_ENTRIES];
	schedule_list_t(): size(0) {}
	schedule_list_t(uint8_t size): size(size) {}
};

/** Header of a stream buffer
 *
 */
struct __attribute__((__packed__)) stream_buffer_header_t {
	uint8_t type;
	uint8_t opCode; //! can be used as op code, see <OpCode>
	uint16_t length;
};

enum class OperationMode {
	OPERATION_MODE_SETUP                       = 0x00,
	OPERATION_MODE_DFU                         = 0x01,
	OPERATION_MODE_FACTORY_RESET               = 0x02,
	OPERATION_MODE_NORMAL                      = 0x10,
	OPERATION_MODE_UNINITIALIZED               = 0xFF,
};

struct __attribute__((packed)) reset_delayed_t {
	uint8_t resetCode;
	uint16_t delayMs;
};




struct event_t {
	event_t(CS_TYPE type, void * data, size16_t size): type(type), data(data), size(size) {
	}

	event_t(CS_TYPE type): type(type), data(NULL), size(0) {
	}

	CS_TYPE type;

	void *data;

	size16_t size;
};

typedef uint16_t cs_file_id_t;

union st_value_t {
	int8_t    s8;
	int16_t   s16;
	int32_t   s32;
	uint8_t   u8;
	uint16_t  u16;
	uint32_t  u32;
} __ALIGN(4);

struct cs_file_data_t {
	CS_TYPE type;
	uint8_t *value;
	uint16_t size;

	friend bool operator==(const cs_file_data_t data0, const cs_file_data_t & data1) {
		if (data0.type != data1.type || data0.size != data1.size) {
			return false;
		}
		for (int i = 0; i < data0.size; ++i) {
			if (data0.value[i] != data1.value[i]) {
				return false;
			}
		}
		return true;
	}
	friend bool operator!=(const cs_file_data_t data0, const cs_file_data_t & data1) {
		return !(data0 == data1);
	}

};

/*---------------------------------------------------------------------------------------------------------------------
 *
 *                                               Types
 *
 *-------------------------------------------------------------------------------------------------------------------*/

#ifndef TYPIFY
#define TYPIFY(NAME) NAME ## _TYPE
#endif
//TODO: check the types
typedef uint16_t TYPIFY(CONFIG_ADV_INTERVAL);
typedef uint16_t TYPIFY(CONFIG_BOOT_DELAY);
typedef uint16_t TYPIFY(CONFIG_CROWNSTONE_ID);
typedef    float TYPIFY(CONFIG_CURRENT_MULTIPLIER);
typedef  int32_t TYPIFY(CONFIG_CURRENT_ADC_ZERO);
typedef     bool TYPIFY(CONFIG_DEFAULT_ON);
typedef     bool TYPIFY(CONFIG_ENCRYPTION_ENABLED);
typedef     bool TYPIFY(CONFIG_IBEACON_ENABLED);
typedef uint16_t TYPIFY(CONFIG_IBEACON_MINOR);
typedef uint16_t TYPIFY(CONFIG_IBEACON_MAJOR);
typedef   int8_t TYPIFY(CONFIG_IBEACON_TXPOWER);
typedef   int8_t TYPIFY(CONFIG_LOW_TX_POWER);
typedef   int8_t TYPIFY(CONFIG_MAX_CHIP_TEMP);
typedef     bool TYPIFY(CONFIG_MESH_ENABLED);
typedef  int32_t TYPIFY(CONFIG_POWER_ZERO);
typedef uint32_t TYPIFY(CONFIG_PWM_PERIOD);
typedef    float TYPIFY(CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP);
typedef    float TYPIFY(CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN);
typedef     bool TYPIFY(CONFIG_PWM_ALLOWED);
typedef uint16_t TYPIFY(CONFIG_RELAY_HIGH_DURATION);
typedef     bool TYPIFY(CONFIG_SCANNER_ENABLED);
typedef uint16_t TYPIFY(CONFIG_SCAN_BREAK_DURATION);
typedef uint16_t TYPIFY(CONFIG_SCAN_DURATION);
typedef uint16_t TYPIFY(CONFIG_SCAN_INTERVAL);
typedef uint16_t TYPIFY(CONFIG_SCAN_WINDOW);
typedef uint16_t TYPIFY(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD);
typedef uint16_t TYPIFY(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM);
typedef     bool TYPIFY(CONFIG_SWITCH_LOCKED);
typedef     bool TYPIFY(CONFIG_SWITCHCRAFT_ENABLED);
typedef    float TYPIFY(CONFIG_SWITCHCRAFT_THRESHOLD);
typedef   int8_t TYPIFY(CONFIG_TX_POWER);
typedef  uint8_t TYPIFY(CONFIG_UART_ENABLED);
typedef    float TYPIFY(CONFIG_VOLTAGE_MULTIPLIER);
typedef  int32_t TYPIFY(CONFIG_VOLTAGE_ADC_ZERO);

typedef  int32_t TYPIFY(STATE_ACCUMULATED_ENERGY);
typedef state_errors_t TYPIFY(STATE_ERRORS);
typedef     bool TYPIFY(STATE_ERROR_CHIP_TEMP);
typedef     bool TYPIFY(STATE_ERROR_DIMMER_OFF_FAILURE);
typedef     bool TYPIFY(STATE_ERROR_DIMMER_ON_FAILURE);
typedef     bool TYPIFY(STATE_ERROR_OVER_CURRENT);
typedef     bool TYPIFY(STATE_ERROR_OVER_CURRENT_DIMMER);
typedef     bool TYPIFY(STATE_ERROR_DIMMER_TEMP);
typedef  uint8_t TYPIFY(STATE_FACTORY_RESET);
typedef  uint8_t TYPIFY(STATE_OPERATION_MODE);
typedef  int32_t TYPIFY(STATE_POWER_USAGE);
typedef uint16_t TYPIFY(STATE_RESET_COUNTER);
typedef schedule_list_t TYPIFY(STATE_SCHEDULE);
typedef switch_state_t TYPIFY(STATE_SWITCH_STATE);
typedef   int8_t TYPIFY(STATE_TEMPERATURE);
typedef uint32_t TYPIFY(STATE_TIME);

typedef  void TYPIFY(EVT_ADC_RESTARTED);
typedef  void TYPIFY(EVT_ADVERTISEMENT_UPDATED);
typedef  void TYPIFY(EVT_BLE_CONNECT);
typedef  void TYPIFY(EVT_BLE_DISCONNECT);
typedef  void TYPIFY(EVT_BROWNOUT_IMPENDING);
//typedef  uint8_t TYPIFY(EVT_CHARACTERISTICS_UPDATED);
typedef  void TYPIFY(EVT_CHIP_TEMP_ABOVE_THRESHOLD);
typedef  void TYPIFY(EVT_CHIP_TEMP_OK);
typedef reset_delayed_t TYPIFY(CMD_RESET_DELAYED);
typedef  OperationMode TYPIFY(CMD_SET_OPERATION_MODE);
typedef  void TYPIFY(EVT_CURRENT_USAGE_ABOVE_THRESHOLD_DIMMER);
typedef  void TYPIFY(EVT_CURRENT_USAGE_ABOVE_THRESHOLD);
typedef  void TYPIFY(CMD_DEC_CURRENT_RANGE);
typedef  void TYPIFY(CMD_DEC_VOLTAGE_RANGE);
typedef  scanned_device_t TYPIFY(EVT_DEVICE_SCANNED);
typedef  void TYPIFY(EVT_DIMMER_ON_FAILURE_DETECTED);
typedef  void TYPIFY(EVT_DIMMER_OFF_FAILURE_DETECTED);
//typedef  uint8_t TYPIFY(EVT_ENABLED_MESH);
//typedef  uint8_t TYPIFY(EVT_ENABLED_ENCRYPTION);
//typedef  uint8_t TYPIFY(EVT_ENABLED_IBEACON);
typedef  bool TYPIFY(CMD_ENABLE_ADC_DIFFERENTIAL_CURRENT);
typedef  bool TYPIFY(CMD_ENABLE_ADC_DIFFERENTIAL_VOLTAGE);
typedef  bool TYPIFY(CMD_ENABLE_ADVERTISEMENT);
typedef  bool TYPIFY(CMD_ENABLE_LOG_CURRENT);
typedef  bool TYPIFY(CMD_ENABLE_LOG_FILTERED_CURRENT);
typedef  bool TYPIFY(CMD_ENABLE_LOG_POWER);
typedef  bool TYPIFY(CMD_ENABLE_LOG_VOLTAGE);
typedef  bool TYPIFY(CMD_ENABLE_MESH);
typedef  void TYPIFY(CMD_INC_VOLTAGE_RANGE);
typedef  void TYPIFY(CMD_INC_CURRENT_RANGE);
typedef  void TYPIFY(EVT_KEEP_ALIVE);
typedef uint32_t TYPIFY(EVT_MESH_TIME);
//typedef  uint8_t TYPIFY(EVT_POWER_CONSUMPTION);
typedef  void TYPIFY(CMD_SWITCH_OFF);
typedef  void TYPIFY(CMD_SWITCH_ON);
typedef  void TYPIFY(EVT_POWER_SAMPLES_END);
typedef  void TYPIFY(EVT_POWER_SAMPLES_START);
typedef  void TYPIFY(CMD_SWITCH_TOGGLE);
typedef  bool TYPIFY(EVT_DIMMING_ALLOWED);
typedef  void TYPIFY(EVT_DIMMER_FORCED_OFF);
typedef  void TYPIFY(EVT_DIMMER_POWERED);
typedef  void TYPIFY(EVT_DIMMER_TEMP_ABOVE_THRESHOLD);
typedef  void TYPIFY(EVT_DIMMER_TEMP_OK);
typedef  void TYPIFY(EVT_RELAY_FORCED_ON);
typedef  stream_buffer_header_t TYPIFY(CMD_CONTROL_CMD);
typedef  void TYPIFY(EVT_SCAN_STARTED);
typedef  void TYPIFY(EVT_SCAN_STOPPED);
typedef  schedule_list_t TYPIFY(EVT_SCHEDULE_ENTRIES_UPDATED);
typedef  void TYPIFY(EVT_SETUP_DONE);
typedef  session_nonce_t TYPIFY(EVT_SESSION_NONCE_SET);
typedef  CS_TYPE TYPIFY(EVT_STORAGE_WRITE_DONE);
typedef  bool TYPIFY(EVT_SWITCHCRAFT_ENABLED);
typedef  void TYPIFY(EVT_SWITCH_FORCED_OFF);
typedef  bool TYPIFY(EVT_SWITCH_LOCKED);
typedef  void TYPIFY(EVT_TIME_SET);
typedef  void TYPIFY(CMD_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN);

/*---------------------------------------------------------------------------------------------------------------------
 *
 *                                               Sizes
 *
 *-------------------------------------------------------------------------------------------------------------------*/

/**
 * Store values in FLASH or RAM. Load values from FIRMWARE_DEFAULT, FLASH or RAM.
 *
 * 1. Values that are written fairly often and are not important over reboots should be stored in and read from RAM.
 * For example, we can measure continuously the temperature of the chip. We can also all the time read this value
 * from one of the BLE services. There is no reason to do a roundtrip to FLASH.
 *
 * 2. Values like CONFIG_BOOT_DELAY should be known over reboots of the device. Moreover, they should also persist
 * over firmware updates. These values are stored in FLASH. If the values are actually not changed by the user (or via
 * the smartphone app), they should NOT be stored to FLASH. They can then immediately be read from the
 * FIRMWARE_DEFAULT. They will be stored only if they are explicitly overwritten. If these values are stored in FLASH
 * they always take precedence over FIRMWARE_DEFAULT values.
 *
 * 3. Most values can be written to and read from FLASH using RAM as a cache. We can also use FIRMWARE_DEFAULT as
 * a fallback when the FLASH value is not present. Moreover, we can have a list that specifies if a value should be
 * in RAM or in FLASH by default. This complete persistence strategy is called STRATEGY1.
 *
 * NOTE. Suppose we have a new firmware available and we definitely want to use a new FIRMWARE_DEFAULT value. For
 * example, we use more peripherals and need to have a CONFIG_BOOT_DELAY that is higher or else it will be in an
 * infinite reboot loop. Before we upload the new firmware to the Crownstone, we need to explicitly clear the value.
 * Only after we have deleted the FLASH record we can upload the new firmware. Then the new FIRMWARE_DEFAULT is used
 * automatically.
 */
enum class PersistenceMode: uint8_t {
	FLASH, // TODO: Do we have a use case for this mode?
	RAM,
	FIRMWARE_DEFAULT,
	STRATEGY1,
};

/**
 * The size of a particular default value. In case of strings or arrays this is the maximum size of the corresponding
 * field. There are no fields that are of unrestricted size. For fields that are not implemented it is possible to
 * set size to 0.
 */
constexpr size16_t TypeSize(CS_TYPE const & type) {
	switch(type) {
	case CS_TYPE::CONFIG_DO_NOT_USE:
		return 0;
	case CS_TYPE::CONFIG_NAME:
		return MAX_STRING_STORAGE_SIZE+1;
	case CS_TYPE::CONFIG_PWM_PERIOD:
		return sizeof(TYPIFY(CONFIG_PWM_PERIOD));
	case CS_TYPE::CONFIG_IBEACON_MAJOR:
		return sizeof(TYPIFY(CONFIG_IBEACON_MAJOR));
	case CS_TYPE::CONFIG_IBEACON_MINOR:
		return sizeof(TYPIFY(CONFIG_IBEACON_MINOR));
	case CS_TYPE::CONFIG_IBEACON_UUID:
		return 16;
	case CS_TYPE::CONFIG_IBEACON_TXPOWER:
		return sizeof(TYPIFY(CONFIG_IBEACON_TXPOWER));
	case CS_TYPE::CONFIG_TX_POWER:
		return sizeof(TYPIFY(CONFIG_TX_POWER));
	case CS_TYPE::CONFIG_ADV_INTERVAL:
		return sizeof(TYPIFY(CONFIG_ADV_INTERVAL));
	case CS_TYPE::CONFIG_SCAN_DURATION:
		return sizeof(TYPIFY(CONFIG_SCAN_DURATION));
	case CS_TYPE::CONFIG_SCAN_BREAK_DURATION:
		return sizeof(TYPIFY(CONFIG_SCAN_BREAK_DURATION));
	case CS_TYPE::CONFIG_BOOT_DELAY:
		return sizeof(TYPIFY(CONFIG_BOOT_DELAY));
	case CS_TYPE::CONFIG_MAX_CHIP_TEMP:
		return sizeof(TYPIFY(CONFIG_MAX_CHIP_TEMP));
	case CS_TYPE::CONFIG_CURRENT_LIMIT:
		return 0; // Not implemented
	case CS_TYPE::CONFIG_MESH_ENABLED:
		return sizeof(TYPIFY(CONFIG_MESH_ENABLED));
	case CS_TYPE::CONFIG_ENCRYPTION_ENABLED:
		return sizeof(TYPIFY(CONFIG_ENCRYPTION_ENABLED));
	case CS_TYPE::CONFIG_IBEACON_ENABLED:
		return sizeof(TYPIFY(CONFIG_IBEACON_ENABLED));
	case CS_TYPE::CONFIG_SCANNER_ENABLED:
		return sizeof(TYPIFY(CONFIG_SCANNER_ENABLED));
	case CS_TYPE::CONFIG_CROWNSTONE_ID:
		return sizeof(TYPIFY(CONFIG_CROWNSTONE_ID));
	case CS_TYPE::CONFIG_KEY_ADMIN:
		return ENCRYPTION_KEY_LENGTH;
	case CS_TYPE::CONFIG_KEY_MEMBER:
		return ENCRYPTION_KEY_LENGTH;
	case CS_TYPE::CONFIG_KEY_GUEST:
		return ENCRYPTION_KEY_LENGTH;
	case CS_TYPE::CONFIG_DEFAULT_ON:
		return sizeof(TYPIFY(CONFIG_DEFAULT_ON));
	case CS_TYPE::CONFIG_SCAN_INTERVAL:
		return sizeof(TYPIFY(CONFIG_SCAN_INTERVAL));
	case CS_TYPE::CONFIG_SCAN_WINDOW:
		return sizeof(TYPIFY(CONFIG_SCAN_WINDOW));
	case CS_TYPE::CONFIG_RELAY_HIGH_DURATION:
		return sizeof(TYPIFY(CONFIG_RELAY_HIGH_DURATION));
	case CS_TYPE::CONFIG_LOW_TX_POWER:
		return sizeof(TYPIFY(CONFIG_LOW_TX_POWER));
	case CS_TYPE::CONFIG_VOLTAGE_MULTIPLIER:
		return sizeof(TYPIFY(CONFIG_VOLTAGE_MULTIPLIER));
	case CS_TYPE::CONFIG_CURRENT_MULTIPLIER:
		return sizeof(TYPIFY(CONFIG_CURRENT_MULTIPLIER));
	case CS_TYPE::CONFIG_VOLTAGE_ADC_ZERO:
		return 0; // TODO
	case CS_TYPE::CONFIG_CURRENT_ADC_ZERO:
		return sizeof(TYPIFY(CONFIG_CURRENT_ADC_ZERO));
	case CS_TYPE::CONFIG_POWER_ZERO:
		return sizeof(TYPIFY(CONFIG_POWER_ZERO));
	case CS_TYPE::CONFIG_SOFT_FUSE_CURRENT_THRESHOLD:
		return sizeof(TYPIFY(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD));
	case CS_TYPE::CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM:
		return sizeof(TYPIFY(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM));
	case CS_TYPE::CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP:
		return sizeof(TYPIFY(CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP));
	case CS_TYPE::CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN:
		return sizeof(TYPIFY(CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN));
	case CS_TYPE::CONFIG_PWM_ALLOWED:
		return sizeof(TYPIFY(CONFIG_PWM_ALLOWED));
	case CS_TYPE::CONFIG_SWITCH_LOCKED:
		return sizeof(TYPIFY(CONFIG_SWITCH_LOCKED));
	case CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED:
		return sizeof(TYPIFY(CONFIG_SWITCHCRAFT_ENABLED));
	case CS_TYPE::CONFIG_SWITCHCRAFT_THRESHOLD:
		return sizeof(TYPIFY(CONFIG_SWITCHCRAFT_THRESHOLD));
	case CS_TYPE::CONFIG_UART_ENABLED:
		return sizeof(TYPIFY(CONFIG_UART_ENABLED));
	case CS_TYPE::STATE_RESET_COUNTER:
		return sizeof(TYPIFY(STATE_RESET_COUNTER));
	case CS_TYPE::STATE_SWITCH_STATE:
		return sizeof(TYPIFY(STATE_SWITCH_STATE));
	case CS_TYPE::STATE_ACCUMULATED_ENERGY:
		return sizeof(TYPIFY(STATE_ACCUMULATED_ENERGY));
	case CS_TYPE::STATE_POWER_USAGE:
		return sizeof(TYPIFY(STATE_POWER_USAGE));
	case CS_TYPE::STATE_SCHEDULE:
		return sizeof(TYPIFY(STATE_SCHEDULE));
	case CS_TYPE::STATE_OPERATION_MODE:
		return sizeof(TYPIFY(STATE_OPERATION_MODE));
	case CS_TYPE::STATE_TEMPERATURE:
		return sizeof(TYPIFY(STATE_TEMPERATURE));
	case CS_TYPE::STATE_TIME:
		return sizeof(TYPIFY(STATE_TIME));
	case CS_TYPE::STATE_FACTORY_RESET:
		return sizeof(TYPIFY(STATE_FACTORY_RESET));
	case CS_TYPE::STATE_ERRORS:
		return sizeof(TYPIFY(STATE_ERRORS));
	case CS_TYPE::STATE_ERROR_OVER_CURRENT:
		return sizeof(TYPIFY(STATE_ERROR_OVER_CURRENT));
	case CS_TYPE::STATE_ERROR_OVER_CURRENT_DIMMER:
		return sizeof(TYPIFY(STATE_ERROR_OVER_CURRENT_DIMMER));
	case CS_TYPE::STATE_ERROR_CHIP_TEMP:
		return sizeof(TYPIFY(STATE_ERROR_CHIP_TEMP));
	case CS_TYPE::STATE_ERROR_DIMMER_TEMP:
		return sizeof(TYPIFY(STATE_ERROR_DIMMER_TEMP));
	case CS_TYPE::STATE_ERROR_DIMMER_ON_FAILURE:
		return sizeof(TYPIFY(STATE_ERROR_DIMMER_ON_FAILURE));
	case CS_TYPE::STATE_ERROR_DIMMER_OFF_FAILURE:
		return sizeof(TYPIFY(STATE_ERROR_DIMMER_OFF_FAILURE));
	case CS_TYPE::CMD_SWITCH_OFF:
		return 0;
	case CS_TYPE::CMD_SWITCH_ON:
		return 0;
	case CS_TYPE::CMD_SWITCH_TOGGLE:
		return 0;
	case CS_TYPE::EVT_ADVERTISEMENT_UPDATED:
		return 0;
	case CS_TYPE::EVT_SCAN_STARTED:
		return 0;
	case CS_TYPE::EVT_SCAN_STOPPED:
		return 0;
	case CS_TYPE::EVT_DEVICE_SCANNED:
		return sizeof(TYPIFY(EVT_DEVICE_SCANNED));
	case CS_TYPE::EVT_POWER_SAMPLES_START:
		return 0;
	case CS_TYPE::EVT_POWER_SAMPLES_END:
		return 0;
	case CS_TYPE::EVT_CURRENT_USAGE_ABOVE_THRESHOLD_DIMMER:
		return 0;
	case CS_TYPE::EVT_CURRENT_USAGE_ABOVE_THRESHOLD:
		return 0;
	case CS_TYPE::EVT_DIMMER_ON_FAILURE_DETECTED:
		return 0;
	case CS_TYPE::EVT_DIMMER_OFF_FAILURE_DETECTED:
		return 0;
	//case CS_TYPE::EVT_POWER_CONSUMPTION:
	//	return sizeof(TYPIFY(EVT_POWER_CONSUMPTION));
	//case CS_TYPE::EVT_ENABLED_MESH:
	//	return sizeof(TYPIFY(EVT_ENABLED_MESH));
	//case CS_TYPE::EVT_ENABLED_ENCRYPTION:
	//	return sizeof(TYPIFY(EVT_ENABLED_ENCRYPTION));
	//case CS_TYPE::EVT_ENABLED_IBEACON:
	//	return sizeof(TYPIFY(EVT_ENABLED_IBEACON));
	//case CS_TYPE::EVT_CHARACTERISTICS_UPDATED:
	//	return sizeof(TYPIFY(EVT_CHARACTERISTICS_UPDATED));
	case CS_TYPE::EVT_MESH_TIME:
		return sizeof(TYPIFY(EVT_MESH_TIME));
	case CS_TYPE::EVT_SCHEDULE_ENTRIES_UPDATED:
		return sizeof(TYPIFY(EVT_SCHEDULE_ENTRIES_UPDATED));
	case CS_TYPE::EVT_BLE_CONNECT:
		return 0;
	case CS_TYPE::EVT_BLE_DISCONNECT:
		return 0;
	case CS_TYPE::EVT_BROWNOUT_IMPENDING:
		return 0;
	case CS_TYPE::EVT_SESSION_NONCE_SET:
		return sizeof(TYPIFY(EVT_SESSION_NONCE_SET));
	case CS_TYPE::EVT_KEEP_ALIVE:
		return 0;
	case CS_TYPE::EVT_DIMMER_FORCED_OFF:
		return 0;
	case CS_TYPE::EVT_SWITCH_FORCED_OFF:
		return 0;
	case CS_TYPE::EVT_RELAY_FORCED_ON:
		return 0;
	case CS_TYPE::EVT_CHIP_TEMP_ABOVE_THRESHOLD:
		return 0;
	case CS_TYPE::EVT_CHIP_TEMP_OK:
		return 0;
	case CS_TYPE::EVT_DIMMER_TEMP_ABOVE_THRESHOLD:
		return 0;
	case CS_TYPE::EVT_DIMMER_TEMP_OK:
		return 0;
	case CS_TYPE::EVT_TIME_SET:
		return 0;
	case CS_TYPE::EVT_DIMMER_POWERED:
		return 0;
	case CS_TYPE::EVT_DIMMING_ALLOWED:
		return sizeof(TYPIFY(EVT_DIMMING_ALLOWED));
	case CS_TYPE::EVT_SWITCH_LOCKED:
		return sizeof(TYPIFY(EVT_SWITCH_LOCKED));
	case CS_TYPE::EVT_STORAGE_WRITE_DONE:
		return sizeof(TYPIFY(EVT_STORAGE_WRITE_DONE));
	case CS_TYPE::EVT_SETUP_DONE:
		return 0;
	case CS_TYPE::EVT_SWITCHCRAFT_ENABLED:
		return sizeof(TYPIFY(EVT_SWITCHCRAFT_ENABLED));
	case CS_TYPE::EVT_ADC_RESTARTED:
		return 0;
	case CS_TYPE::CMD_ENABLE_LOG_POWER:
		return sizeof(TYPIFY(CMD_ENABLE_LOG_POWER));
	case CS_TYPE::CMD_ENABLE_LOG_CURRENT:
		return sizeof(TYPIFY(CMD_ENABLE_LOG_CURRENT));
	case CS_TYPE::CMD_ENABLE_LOG_VOLTAGE:
		return sizeof(TYPIFY(CMD_ENABLE_LOG_VOLTAGE));
	case CS_TYPE::CMD_ENABLE_LOG_FILTERED_CURRENT:
		return sizeof(TYPIFY(CMD_ENABLE_LOG_FILTERED_CURRENT));
	case CS_TYPE::CMD_RESET_DELAYED:
		return sizeof(TYPIFY(CMD_RESET_DELAYED));
	case CS_TYPE::CMD_ENABLE_ADVERTISEMENT:
		return sizeof(TYPIFY(CMD_ENABLE_ADVERTISEMENT));
	case CS_TYPE::CMD_ENABLE_MESH:
		return sizeof(TYPIFY(CMD_ENABLE_MESH));
	case CS_TYPE::CMD_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN:
		return 0;
	case CS_TYPE::CMD_ENABLE_ADC_DIFFERENTIAL_CURRENT:
		return sizeof(TYPIFY(CMD_ENABLE_ADC_DIFFERENTIAL_CURRENT));
	case CS_TYPE::CMD_ENABLE_ADC_DIFFERENTIAL_VOLTAGE:
		return sizeof(TYPIFY(CMD_ENABLE_ADC_DIFFERENTIAL_VOLTAGE));
	case CS_TYPE::CMD_INC_VOLTAGE_RANGE:
		return 0;
	case CS_TYPE::CMD_DEC_VOLTAGE_RANGE:
		return 0;
	case CS_TYPE::CMD_INC_CURRENT_RANGE:
		return 0;
	case CS_TYPE::CMD_DEC_CURRENT_RANGE:
		return 0;
	case CS_TYPE::CMD_CONTROL_CMD:
		return sizeof(TYPIFY(CMD_CONTROL_CMD));
	case CS_TYPE::CMD_SET_OPERATION_MODE:
		return sizeof(TYPIFY(CMD_SET_OPERATION_MODE));
	}
	// should never happen
	return 0;
}

/*---------------------------------------------------------------------------------------------------------------------
 *
 *                                               Names
 *
 *-------------------------------------------------------------------------------------------------------------------*/

constexpr const char* TypeName(CS_TYPE const & type) {

	switch(type) {
	case CS_TYPE::CONFIG_DO_NOT_USE: return "CONFIG_DO_NOT_USE";
	case CS_TYPE::CONFIG_ADV_INTERVAL: return "CONFIG_ADV_INTERVAL";
	case CS_TYPE::CONFIG_BOOT_DELAY: return "CONFIG_BOOT_DELAY";
	case CS_TYPE::CONFIG_CROWNSTONE_ID: return "CONFIG_CROWNSTONE_ID";
	case CS_TYPE::CONFIG_CURRENT_MULTIPLIER: return "CONFIG_CURRENT_MULTIPLIER";
	case CS_TYPE::CONFIG_CURRENT_ADC_ZERO: return "CONFIG_CURRENT_ADC_ZERO";
	case CS_TYPE::CONFIG_CURRENT_LIMIT: return "CONFIG_CURRENT_LIMIT";
	case CS_TYPE::CONFIG_DEFAULT_ON: return "CONFIG_DEFAULT_ON";
	case CS_TYPE::CONFIG_ENCRYPTION_ENABLED: return "CONFIG_ENCRYPTION_ENABLED";
	case CS_TYPE::CONFIG_IBEACON_ENABLED: return "CONFIG_IBEACON_ENABLED";
	case CS_TYPE::CONFIG_IBEACON_MAJOR: return "CONFIG_IBEACON_MAJOR";
	case CS_TYPE::CONFIG_IBEACON_MINOR: return "CONFIG_IBEACON_MINOR";
	case CS_TYPE::CONFIG_IBEACON_TXPOWER: return "CONFIG_IBEACON_TXPOWER";
	case CS_TYPE::CONFIG_IBEACON_UUID: return "CONFIG_IBEACON_UUID";
	case CS_TYPE::CONFIG_KEY_ADMIN: return "CONFIG_KEY_ADMIN";
	case CS_TYPE::CONFIG_KEY_GUEST: return "CONFIG_KEY_GUEST";
	case CS_TYPE::CONFIG_KEY_MEMBER: return "CONFIG_KEY_MEMBER";
	case CS_TYPE::CONFIG_LOW_TX_POWER: return "CONFIG_LOW_TX_POWER";
	case CS_TYPE::CONFIG_MAX_CHIP_TEMP: return "CONFIG_MAX_CHIP_TEMP";
	case CS_TYPE::CONFIG_MESH_ENABLED: return "CONFIG_MESH_ENABLED";
	case CS_TYPE::CONFIG_NAME: return "CONFIG_NAME";
	case CS_TYPE::CONFIG_POWER_ZERO: return "CONFIG_POWER_ZERO";
	case CS_TYPE::CONFIG_PWM_ALLOWED: return "CONFIG_PWM_ALLOWED";
	case CS_TYPE::CONFIG_PWM_PERIOD: return "CONFIG_PWM_PERIOD";
	case CS_TYPE::CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN: return "CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN";
	case CS_TYPE::CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP: return "CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP";
	case CS_TYPE::CONFIG_RELAY_HIGH_DURATION: return "CONFIG_RELAY_HIGH_DURATION";
	case CS_TYPE::CONFIG_SCANNER_ENABLED: return "CONFIG_SCANNER_ENABLED";
	case CS_TYPE::CONFIG_SCAN_BREAK_DURATION: return "CONFIG_SCAN_BREAK_DURATION";
	case CS_TYPE::CONFIG_SCAN_DURATION: return "CONFIG_SCAN_DURATION";
	case CS_TYPE::CONFIG_SCAN_INTERVAL: return "CONFIG_SCAN_INTERVAL";
	case CS_TYPE::CONFIG_SCAN_WINDOW: return "CONFIG_SCAN_WINDOW";
	case CS_TYPE::CONFIG_SOFT_FUSE_CURRENT_THRESHOLD: return "CONFIG_SOFT_FUSE_CURRENT_THRESHOLD";
	case CS_TYPE::CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM: return "CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM";
	case CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED: return "CONFIG_SWITCHCRAFT_ENABLED";
	case CS_TYPE::CONFIG_SWITCHCRAFT_THRESHOLD: return "CONFIG_SWITCHCRAFT_THRESHOLD";
	case CS_TYPE::CONFIG_SWITCH_LOCKED: return "CONFIG_SWITCH_LOCKED";
	case CS_TYPE::CONFIG_TX_POWER: return "CONFIG_TX_POWER";
	case CS_TYPE::CONFIG_UART_ENABLED: return "CONFIG_UART_ENABLED";
	case CS_TYPE::CONFIG_VOLTAGE_MULTIPLIER: return "CONFIG_VOLTAGE_MULTIPLIER";
	case CS_TYPE::CONFIG_VOLTAGE_ADC_ZERO: return "CONFIG_VOLTAGE_ADC_ZERO";
	case CS_TYPE::EVT_ADC_RESTARTED: return "EVT_ADC_RESTARTED";
	case CS_TYPE::EVT_ADVERTISEMENT_UPDATED: return "EVT_ADVERTISEMENT_UPDATED";
	case CS_TYPE::EVT_BLE_CONNECT: return "EVT_BLE_CONNECT";
	case CS_TYPE::EVT_BLE_DISCONNECT: return "EVT_BLE_DISCONNECT";
	case CS_TYPE::EVT_BROWNOUT_IMPENDING: return "EVT_BROWNOUT_IMPENDING";
	case CS_TYPE::EVT_CHIP_TEMP_ABOVE_THRESHOLD: return "EVT_CHIP_TEMP_ABOVE_THRESHOLD";
	case CS_TYPE::EVT_CHIP_TEMP_OK: return "EVT_CHIP_TEMP_OK";
	case CS_TYPE::CMD_RESET_DELAYED: return "EVT_CMD_RESET";
	case CS_TYPE::CMD_SET_OPERATION_MODE: return "CMD_SET_OPERATION_MODE";
	case CS_TYPE::EVT_CURRENT_USAGE_ABOVE_THRESHOLD: return "EVT_CURRENT_USAGE_ABOVE_THRESHOLD";
	case CS_TYPE::EVT_CURRENT_USAGE_ABOVE_THRESHOLD_DIMMER: return "EVT_CURRENT_USAGE_ABOVE_THRESHOLD_PWM";
	case CS_TYPE::CMD_DEC_CURRENT_RANGE: return "EVT_DEC_CURRENT_RANGE";
	case CS_TYPE::CMD_DEC_VOLTAGE_RANGE: return "EVT_DEC_VOLTAGE_RANGE";
	case CS_TYPE::EVT_DEVICE_SCANNED: return "EVT_DEVICE_SCANNED";
	case CS_TYPE::EVT_DIMMER_OFF_FAILURE_DETECTED: return "EVT_DIMMER_OFF_FAILURE_DETECTED";
	case CS_TYPE::EVT_DIMMER_ON_FAILURE_DETECTED: return "EVT_DIMMER_ON_FAILURE_DETECTED";
	case CS_TYPE::CMD_ENABLE_ADC_DIFFERENTIAL_CURRENT: return "EVT_ENABLE_ADC_DIFFERENTIAL_CURRENT";
	case CS_TYPE::CMD_ENABLE_ADC_DIFFERENTIAL_VOLTAGE: return "EVT_ENABLE_ADC_DIFFERENTIAL_VOLTAGE";
	case CS_TYPE::CMD_ENABLE_ADVERTISEMENT: return "EVT_ENABLE_ADVERTISEMENT";
	case CS_TYPE::CMD_ENABLE_LOG_CURRENT: return "EVT_ENABLE_LOG_CURRENT";
	case CS_TYPE::CMD_ENABLE_LOG_FILTERED_CURRENT: return "EVT_ENABLE_LOG_FILTERED_CURRENT";
	case CS_TYPE::CMD_ENABLE_LOG_POWER: return "EVT_ENABLE_LOG_POWER";
	case CS_TYPE::CMD_ENABLE_LOG_VOLTAGE: return "EVT_ENABLE_LOG_VOLTAGE";
	case CS_TYPE::CMD_ENABLE_MESH: return "EVT_ENABLE_MESH";
	case CS_TYPE::CMD_INC_CURRENT_RANGE: return "EVT_INC_CURRENT_RANGE";
	case CS_TYPE::CMD_INC_VOLTAGE_RANGE: return "EVT_INC_VOLTAGE_RANGE";
	case CS_TYPE::EVT_KEEP_ALIVE: return "EVT_KEEP_ALIVE";
	case CS_TYPE::EVT_MESH_TIME: return "EVT_MESH_TIME";
	case CS_TYPE::CMD_SWITCH_OFF: return "EVT_POWER_OFF";
	case CS_TYPE::CMD_SWITCH_ON: return "EVT_POWER_ON";
	case CS_TYPE::EVT_POWER_SAMPLES_END: return "EVT_POWER_SAMPLES_END";
	case CS_TYPE::EVT_POWER_SAMPLES_START: return "EVT_POWER_SAMPLES_START";
	case CS_TYPE::CMD_SWITCH_TOGGLE: return "EVT_POWER_TOGGLE";
	case CS_TYPE::EVT_DIMMING_ALLOWED: return "EVT_DIMMING_ALLOWED";
	case CS_TYPE::EVT_DIMMER_FORCED_OFF: return "EVT_DIMMER_FORCED_OFF";
	case CS_TYPE::EVT_DIMMER_POWERED: return "EVT_DIMMER_POWERED";
	case CS_TYPE::EVT_DIMMER_TEMP_ABOVE_THRESHOLD: return "EVT_PWM_TEMP_ABOVE_THRESHOLD";
	case CS_TYPE::EVT_DIMMER_TEMP_OK: return "EVT_PWM_TEMP_OK";
	case CS_TYPE::EVT_RELAY_FORCED_ON: return "EVT_RELAY_FORCED_ON";
	case CS_TYPE::CMD_CONTROL_CMD: return "EVT_RX_CONTROL";
	case CS_TYPE::EVT_SCAN_STARTED: return "EVT_SCAN_STARTED";
	case CS_TYPE::EVT_SCAN_STOPPED: return "EVT_SCAN_STOPPED";
	case CS_TYPE::EVT_SCHEDULE_ENTRIES_UPDATED: return "EVT_SCHEDULE_ENTRIES_UPDATED";
	case CS_TYPE::EVT_SESSION_NONCE_SET: return "EVT_SESSION_NONCE_SET";
	case CS_TYPE::EVT_SETUP_DONE: return "EVT_SETUP_DONE";
	case CS_TYPE::EVT_STORAGE_WRITE_DONE: return "EVT_STORAGE_WRITE_DONE";
	case CS_TYPE::EVT_SWITCHCRAFT_ENABLED: return "EVT_SWITCHCRAFT_ENABLED";
	case CS_TYPE::EVT_SWITCH_FORCED_OFF: return "EVT_SWITCH_FORCED_OFF";
	case CS_TYPE::EVT_SWITCH_LOCKED: return "EVT_SWITCH_LOCKED";
	case CS_TYPE::EVT_TIME_SET: return "EVT_TIME_SET";
	case CS_TYPE::CMD_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN: return "EVT_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN";
	case CS_TYPE::STATE_ACCUMULATED_ENERGY: return "STATE_ACCUMULATED_ENERGY";
	case CS_TYPE::STATE_ERRORS: return "STATE_ERRORS";
	case CS_TYPE::STATE_ERROR_CHIP_TEMP: return "STATE_ERROR_CHIP_TEMP";
	case CS_TYPE::STATE_ERROR_DIMMER_OFF_FAILURE: return "STATE_ERROR_DIMMER_OFF_FAILURE";
	case CS_TYPE::STATE_ERROR_DIMMER_ON_FAILURE: return "STATE_ERROR_DIMMER_ON_FAILURE";
	case CS_TYPE::STATE_ERROR_OVER_CURRENT: return "STATE_ERROR_OVER_CURRENT";
	case CS_TYPE::STATE_ERROR_OVER_CURRENT_DIMMER: return "STATE_ERROR_OVER_CURRENT_DIMMER";
	case CS_TYPE::STATE_ERROR_DIMMER_TEMP: return "STATE_ERROR_DIMMER_TEMP";
	case CS_TYPE::STATE_FACTORY_RESET: return "STATE_FACTORY_RESET";
	case CS_TYPE::STATE_OPERATION_MODE: return "STATE_OPERATION_MODE";
	case CS_TYPE::STATE_POWER_USAGE: return "STATE_POWER_USAGE";
	case CS_TYPE::STATE_RESET_COUNTER: return "STATE_RESET_COUNTER";
	case CS_TYPE::STATE_SCHEDULE: return "STATE_SCHEDULE";
	case CS_TYPE::STATE_SWITCH_STATE: return "STATE_SWITCH_STATE";
	case CS_TYPE::STATE_TEMPERATURE: return "STATE_TEMPERATURE";
	case CS_TYPE::STATE_TIME: return "STATE_TIME";
	}
	return "Unknown";
}

constexpr PersistenceMode DefaultLocation(CS_TYPE const & type) {
	switch(type) {
	case CS_TYPE::CONFIG_NAME:
	case CS_TYPE::CONFIG_PWM_PERIOD:
	case CS_TYPE::CONFIG_IBEACON_MAJOR:
	case CS_TYPE::CONFIG_IBEACON_MINOR:
	case CS_TYPE::CONFIG_IBEACON_UUID:
	case CS_TYPE::CONFIG_IBEACON_TXPOWER:
	case CS_TYPE::CONFIG_TX_POWER:
	case CS_TYPE::CONFIG_ADV_INTERVAL:
	case CS_TYPE::CONFIG_SCAN_DURATION:
	case CS_TYPE::CONFIG_SCAN_BREAK_DURATION:
	case CS_TYPE::CONFIG_BOOT_DELAY:
	case CS_TYPE::CONFIG_MAX_CHIP_TEMP:
	case CS_TYPE::CONFIG_CURRENT_LIMIT:
	case CS_TYPE::CONFIG_MESH_ENABLED:
	case CS_TYPE::CONFIG_ENCRYPTION_ENABLED:
	case CS_TYPE::CONFIG_IBEACON_ENABLED:
	case CS_TYPE::CONFIG_SCANNER_ENABLED:
	case CS_TYPE::CONFIG_CROWNSTONE_ID:
	case CS_TYPE::CONFIG_KEY_ADMIN:
	case CS_TYPE::CONFIG_KEY_MEMBER:
	case CS_TYPE::CONFIG_KEY_GUEST:
	case CS_TYPE::CONFIG_DEFAULT_ON:
	case CS_TYPE::CONFIG_SCAN_INTERVAL:
	case CS_TYPE::CONFIG_SCAN_WINDOW:
	case CS_TYPE::CONFIG_RELAY_HIGH_DURATION:
	case CS_TYPE::CONFIG_LOW_TX_POWER:
	case CS_TYPE::CONFIG_VOLTAGE_MULTIPLIER:
	case CS_TYPE::CONFIG_CURRENT_MULTIPLIER:
	case CS_TYPE::CONFIG_VOLTAGE_ADC_ZERO:
	case CS_TYPE::CONFIG_CURRENT_ADC_ZERO:
	case CS_TYPE::CONFIG_POWER_ZERO:
	case CS_TYPE::CONFIG_SOFT_FUSE_CURRENT_THRESHOLD:
	case CS_TYPE::CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM:
	case CS_TYPE::CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP:
	case CS_TYPE::CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN:
	case CS_TYPE::CONFIG_PWM_ALLOWED:
	case CS_TYPE::CONFIG_SWITCH_LOCKED:
	case CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED:
	case CS_TYPE::CONFIG_SWITCHCRAFT_THRESHOLD:
	case CS_TYPE::CONFIG_UART_ENABLED:
	case CS_TYPE::STATE_RESET_COUNTER:
	case CS_TYPE::STATE_OPERATION_MODE:
	case CS_TYPE::STATE_SWITCH_STATE:
	case CS_TYPE::STATE_SCHEDULE:
		return PersistenceMode::STRATEGY1;
	case CS_TYPE::CONFIG_DO_NOT_USE:
	case CS_TYPE::STATE_ACCUMULATED_ENERGY:
	case CS_TYPE::STATE_POWER_USAGE:
	case CS_TYPE::STATE_TEMPERATURE:
	case CS_TYPE::STATE_TIME:
	case CS_TYPE::STATE_FACTORY_RESET:
	case CS_TYPE::STATE_ERRORS:
	case CS_TYPE::STATE_ERROR_OVER_CURRENT:
	case CS_TYPE::STATE_ERROR_OVER_CURRENT_DIMMER:
	case CS_TYPE::STATE_ERROR_CHIP_TEMP:
	case CS_TYPE::STATE_ERROR_DIMMER_TEMP:
	case CS_TYPE::STATE_ERROR_DIMMER_ON_FAILURE:
	case CS_TYPE::STATE_ERROR_DIMMER_OFF_FAILURE:
	case CS_TYPE::CMD_SWITCH_OFF:
	case CS_TYPE::CMD_SWITCH_ON:
	case CS_TYPE::CMD_SWITCH_TOGGLE:
	case CS_TYPE::EVT_ADVERTISEMENT_UPDATED:
	case CS_TYPE::EVT_SCAN_STARTED:
	case CS_TYPE::EVT_SCAN_STOPPED:
	case CS_TYPE::EVT_DEVICE_SCANNED:
	case CS_TYPE::EVT_POWER_SAMPLES_START:
	case CS_TYPE::EVT_POWER_SAMPLES_END:
	case CS_TYPE::EVT_CURRENT_USAGE_ABOVE_THRESHOLD_DIMMER:
	case CS_TYPE::EVT_CURRENT_USAGE_ABOVE_THRESHOLD:
	case CS_TYPE::EVT_DIMMER_ON_FAILURE_DETECTED:
	case CS_TYPE::EVT_DIMMER_OFF_FAILURE_DETECTED:
	//case CS_TYPE::EVT_POWER_CONSUMPTION:
	//case CS_TYPE::EVT_ENABLED_MESH:
	//case CS_TYPE::EVT_ENABLED_ENCRYPTION:
	//case CS_TYPE::EVT_ENABLED_IBEACON:
	//case CS_TYPE::EVT_CHARACTERISTICS_UPDATED:
	case CS_TYPE::EVT_MESH_TIME:
	case CS_TYPE::EVT_SCHEDULE_ENTRIES_UPDATED:
	case CS_TYPE::EVT_BLE_CONNECT:
	case CS_TYPE::EVT_BLE_DISCONNECT:
	case CS_TYPE::EVT_BROWNOUT_IMPENDING:
	case CS_TYPE::EVT_SESSION_NONCE_SET:
	case CS_TYPE::EVT_KEEP_ALIVE:
	case CS_TYPE::EVT_DIMMER_FORCED_OFF:
	case CS_TYPE::EVT_SWITCH_FORCED_OFF:
	case CS_TYPE::EVT_RELAY_FORCED_ON:
	case CS_TYPE::EVT_CHIP_TEMP_ABOVE_THRESHOLD:
	case CS_TYPE::EVT_CHIP_TEMP_OK:
	case CS_TYPE::EVT_DIMMER_TEMP_ABOVE_THRESHOLD:
	case CS_TYPE::EVT_DIMMER_TEMP_OK:
	case CS_TYPE::EVT_TIME_SET:
	case CS_TYPE::EVT_DIMMER_POWERED:
	case CS_TYPE::EVT_DIMMING_ALLOWED:
	case CS_TYPE::EVT_SWITCH_LOCKED:
	case CS_TYPE::EVT_STORAGE_WRITE_DONE:
	case CS_TYPE::EVT_SETUP_DONE:
	case CS_TYPE::EVT_SWITCHCRAFT_ENABLED:
	case CS_TYPE::EVT_ADC_RESTARTED:
	case CS_TYPE::CMD_ENABLE_LOG_POWER:
	case CS_TYPE::CMD_ENABLE_LOG_CURRENT:
	case CS_TYPE::CMD_ENABLE_LOG_VOLTAGE:
	case CS_TYPE::CMD_ENABLE_LOG_FILTERED_CURRENT:
	case CS_TYPE::CMD_RESET_DELAYED:
	case CS_TYPE::CMD_ENABLE_ADVERTISEMENT:
	case CS_TYPE::CMD_ENABLE_MESH:
	case CS_TYPE::CMD_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN:
	case CS_TYPE::CMD_ENABLE_ADC_DIFFERENTIAL_CURRENT:
	case CS_TYPE::CMD_ENABLE_ADC_DIFFERENTIAL_VOLTAGE:
	case CS_TYPE::CMD_INC_VOLTAGE_RANGE:
	case CS_TYPE::CMD_DEC_VOLTAGE_RANGE:
	case CS_TYPE::CMD_INC_CURRENT_RANGE:
	case CS_TYPE::CMD_DEC_CURRENT_RANGE:
	case CS_TYPE::CMD_CONTROL_CMD:
	case CS_TYPE::CMD_SET_OPERATION_MODE:
		return PersistenceMode::RAM;
	}
	// should not reach this
	return PersistenceMode::RAM;
}

/*---------------------------------------------------------------------------------------------------------------------
 *
 *                                               Defaults
 *
 *-------------------------------------------------------------------------------------------------------------------*/

/** Gets the default.
 *
 * Note that if data.value is not aligned at a word boundary, the result still isn't. Use st_value_t to align
 * (u)int8_t, (u)int16_t, float, etc. on word boundaries.
 *
 * There is no allocation done in this function. It is assumed that data.value points to an array or single
 * variable that needs to be written. The allocation of strings or arrays is limited by TypeSize which in that case
 * can be considered as MaxTypeSize.
 */
constexpr void getDefault(cs_file_data_t & data) {

	// for all non-string types we already know the to-be expected size
	data.size = TypeSize(data.type);

	switch(data.type) {
	case CS_TYPE::CONFIG_NAME: {
		data.size = MIN(data.size, sizeof(BLUETOOTH_NAME));
		memcpy(data.value, BLUETOOTH_NAME, data.size);
		break;
	}
	case CS_TYPE::CONFIG_MESH_ENABLED:
		*(TYPIFY(CONFIG_MESH_ENABLED)*)data.value = CONFIG_MESH_DEFAULT;
		break;
	case CS_TYPE::CONFIG_ENCRYPTION_ENABLED:
		*(TYPIFY(CONFIG_ENCRYPTION_ENABLED)*)data.value = CONFIG_ENCRYPTION_DEFAULT;
		break;
	case CS_TYPE::CONFIG_IBEACON_ENABLED:
		*(TYPIFY(CONFIG_IBEACON_ENABLED)*)data.value = CONFIG_IBEACON_DEFAULT;
		break;
	case CS_TYPE::CONFIG_SCANNER_ENABLED:
		*(TYPIFY(CONFIG_SCANNER_ENABLED)*)data.value = CONFIG_SCANNER_DEFAULT;
		break;
	case CS_TYPE::CONFIG_DEFAULT_ON:
		*(TYPIFY(CONFIG_DEFAULT_ON)*)data.value = CONFIG_RELAY_START_DEFAULT;
		break;
	case CS_TYPE::CONFIG_PWM_ALLOWED:
		*(TYPIFY(CONFIG_PWM_ALLOWED)*)data.value = CONFIG_PWM_DEFAULT;
		break;
	case CS_TYPE::CONFIG_SWITCH_LOCKED:
		*(TYPIFY(CONFIG_SWITCH_LOCKED)*)data.value = CONFIG_SWITCH_LOCK_DEFAULT;
		break;
	case CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED:
		*(TYPIFY(CONFIG_SWITCHCRAFT_ENABLED)*)data.value = CONFIG_SWITCHCRAFT_DEFAULT;
		break;
	case CS_TYPE::CONFIG_IBEACON_MAJOR:
		*(TYPIFY(CONFIG_IBEACON_MAJOR)*)data.value = BEACON_MAJOR;
		break;
	case CS_TYPE::CONFIG_IBEACON_MINOR:
		*(TYPIFY(CONFIG_IBEACON_MINOR)*)data.value = BEACON_MINOR;
		break;
	case CS_TYPE::CONFIG_IBEACON_UUID:
		data.size = MIN(data.size, sizeof(STRINGIFY(BEACON_UUID)));
		memcpy(data.value, STRINGIFY(BEACON_UUID), data.size);
		break;
	case CS_TYPE::CONFIG_IBEACON_TXPOWER:
		*(TYPIFY(CONFIG_IBEACON_TXPOWER)*)data.value = BEACON_RSSI;
		break;
	case CS_TYPE::CONFIG_TX_POWER:
		*(TYPIFY(CONFIG_TX_POWER)*)data.value = TX_POWER;
		break;
	case CS_TYPE::CONFIG_ADV_INTERVAL:
		*(TYPIFY(CONFIG_ADV_INTERVAL)*)data.value = ADVERTISEMENT_INTERVAL;
		break;
	case CS_TYPE::CONFIG_SCAN_DURATION:
		*(TYPIFY(CONFIG_SCAN_DURATION)*)data.value = SCAN_DURATION;
		break;
	case CS_TYPE::CONFIG_SCAN_BREAK_DURATION:
		*(TYPIFY(CONFIG_SCAN_BREAK_DURATION)*)data.value = SCAN_BREAK_DURATION;
		break;
	case CS_TYPE::CONFIG_BOOT_DELAY:
		*(TYPIFY(CONFIG_BOOT_DELAY)*)data.value = CONFIG_BOOT_DELAY_DEFAULT;
		break;
	case CS_TYPE::CONFIG_MAX_CHIP_TEMP:
		*(TYPIFY(CONFIG_MAX_CHIP_TEMP)*)data.value = MAX_CHIP_TEMP;
		break;
	case CS_TYPE::CONFIG_CROWNSTONE_ID:
		*(TYPIFY(CONFIG_CROWNSTONE_ID)*)data.value = CONFIG_CROWNSTONE_ID_DEFAULT;
		break;
	case CS_TYPE::CONFIG_KEY_ADMIN:
		break;
	case CS_TYPE::CONFIG_KEY_MEMBER:
		break;
	case CS_TYPE::CONFIG_KEY_GUEST:
		break;
	case CS_TYPE::CONFIG_SCAN_INTERVAL:
		*(TYPIFY(CONFIG_SCAN_INTERVAL)*)data.value = SCAN_INTERVAL;
		break;
	case CS_TYPE::CONFIG_SCAN_WINDOW:
		*(TYPIFY(CONFIG_SCAN_WINDOW)*)data.value = SCAN_WINDOW;
		break;
	case CS_TYPE::CONFIG_RELAY_HIGH_DURATION:
		*(TYPIFY(CONFIG_RELAY_HIGH_DURATION)*)data.value = RELAY_HIGH_DURATION;
		break;
	case CS_TYPE::CONFIG_LOW_TX_POWER:
		*(TYPIFY(CONFIG_LOW_TX_POWER)*)data.value = CONFIG_LOW_TX_POWER_DEFAULT;
		break;
	case CS_TYPE::CONFIG_VOLTAGE_MULTIPLIER:
		*(TYPIFY(CONFIG_VOLTAGE_MULTIPLIER)*)data.value = CONFIG_VOLTAGE_MULTIPLIER_DEFAULT;
		break;
	case CS_TYPE::CONFIG_CURRENT_MULTIPLIER:
		*(TYPIFY(CONFIG_CURRENT_MULTIPLIER)*)data.value = CONFIG_CURRENT_MULTIPLIER_DEFAULT;
		break;
	case CS_TYPE::CONFIG_VOLTAGE_ADC_ZERO:
		*(TYPIFY(CONFIG_VOLTAGE_ADC_ZERO)*)data.value = CONFIG_VOLTAGE_ZERO_DEFAULT;
		break;
	case CS_TYPE::CONFIG_CURRENT_ADC_ZERO:
		*(TYPIFY(CONFIG_CURRENT_ADC_ZERO)*)data.value = CONFIG_CURRENT_ZERO_DEFAULT;
		break;
	case CS_TYPE::CONFIG_POWER_ZERO:
		*(TYPIFY(CONFIG_POWER_ZERO)*)data.value = CONFIG_POWER_ZERO_DEFAULT;
		break;
	case CS_TYPE::CONFIG_PWM_PERIOD:
		LOGd("Got PWM period: %u", PWM_PERIOD);
		LOGd("Data value ptr: %p", data.value);
		*((uint32_t*)data.value) = 1;
		LOGd("data.value: %u", *((uint32_t*)data.value));
		*(TYPIFY(CONFIG_PWM_PERIOD)*)data.value = (TYPIFY(CONFIG_PWM_PERIOD))PWM_PERIOD;
		LOGd("data.value: %u", *((uint32_t*)data.value));
		break;
	case CS_TYPE::CONFIG_SOFT_FUSE_CURRENT_THRESHOLD:
		*(TYPIFY(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD)*)data.value = CURRENT_USAGE_THRESHOLD;
		break;
	case CS_TYPE::CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM:
		*(TYPIFY(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM)*)data.value = CURRENT_USAGE_THRESHOLD_PWM;
		break;
	case CS_TYPE::CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP:
		*(TYPIFY(CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP)*)data.value = CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP_DEFAULT;
		break;
	case CS_TYPE::CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN:
		*(TYPIFY(CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN)*)data.value = CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN_DEFAULT;
		break;
	case CS_TYPE::CONFIG_SWITCHCRAFT_THRESHOLD:
		*(TYPIFY(CONFIG_SWITCHCRAFT_THRESHOLD)*)data.value = SWITCHCRAFT_THRESHOLD;
		break;
	case CS_TYPE::CONFIG_UART_ENABLED:
		*(TYPIFY(CONFIG_UART_ENABLED)*)data.value = CS_SERIAL_ENABLED;
		break;
	case CS_TYPE::STATE_RESET_COUNTER:
		*(TYPIFY(STATE_RESET_COUNTER)*)data.value = STATE_RESET_COUNTER_DEFAULT;
		break;
	case CS_TYPE::STATE_SWITCH_STATE: {
		*(TYPIFY(STATE_SWITCH_STATE)*)data.value = switch_state_t(STATE_SWITCH_STATE_DEFAULT);
		break;
	}
	case CS_TYPE::STATE_ACCUMULATED_ENERGY:
		*(TYPIFY(STATE_ACCUMULATED_ENERGY)*)data.value = STATE_ACCUMULATED_ENERGY_DEFAULT;
		break;
	case CS_TYPE::STATE_POWER_USAGE:
		*(TYPIFY(STATE_POWER_USAGE)*)data.value = STATE_POWER_USAGE_DEFAULT;
		break;
	case CS_TYPE::STATE_SCHEDULE:
		*(TYPIFY(STATE_SCHEDULE)*)data.value = schedule_list_t(STATE_SCHEDULE_DEFAULT); // This is a copy
		break;
	case CS_TYPE::STATE_OPERATION_MODE:
		*(TYPIFY(STATE_OPERATION_MODE)*)data.value = STATE_OPERATION_MODE_DEFAULT;
		break;
	case CS_TYPE::STATE_TEMPERATURE:
		*(TYPIFY(STATE_TEMPERATURE)*)data.value = STATE_TEMPERATURE_DEFAULT;
		break;
	case CS_TYPE::STATE_TIME:
		*(TYPIFY(STATE_TIME)*)data.value = STATE_TIME_DEFAULT;
		break;
	case CS_TYPE::STATE_FACTORY_RESET:
		*(TYPIFY(STATE_FACTORY_RESET)*)data.value = STATE_FACTORY_RESET_DEFAULT;
		break;
	case CS_TYPE::STATE_ERRORS:
		*(TYPIFY(STATE_ERRORS)*)data.value = state_errors_t(STATE_ERRORS_DEFAULT);
		break;
	case CS_TYPE::STATE_ERROR_OVER_CURRENT:
		*(TYPIFY(STATE_ERROR_OVER_CURRENT)*)data.value = STATE_ERROR_OVER_CURRENT_DEFAULT;
		break;
	case CS_TYPE::STATE_ERROR_OVER_CURRENT_DIMMER:
		*(TYPIFY(STATE_ERROR_OVER_CURRENT_DIMMER)*)data.value = STATE_ERROR_OVER_CURRENT_PWM_DEFAULT;
		break;
	case CS_TYPE::STATE_ERROR_CHIP_TEMP:
		*(TYPIFY(STATE_ERROR_CHIP_TEMP)*)data.value = STATE_ERROR_CHIP_TEMP_DEFAULT;
		break;
	case CS_TYPE::STATE_ERROR_DIMMER_TEMP:
		*(TYPIFY(STATE_ERROR_DIMMER_TEMP)*)data.value = STATE_ERROR_PWM_TEMP_DEFAULT;
		break;
	case CS_TYPE::STATE_ERROR_DIMMER_ON_FAILURE:
		*(TYPIFY(STATE_ERROR_DIMMER_ON_FAILURE)*)data.value = STATE_ERROR_DIMMER_ON_FAILURE_DEFAULT;
		break;
	case CS_TYPE::STATE_ERROR_DIMMER_OFF_FAILURE:
		*(TYPIFY(STATE_ERROR_DIMMER_OFF_FAILURE)*)data.value = STATE_ERROR_DIMMER_OFF_FAILURE_DEFAULT;
		break;
	default:
		LOGw("Unknown default for %i", to_underlying_type(data.type));
		break;

	}
}

