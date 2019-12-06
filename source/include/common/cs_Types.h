/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 23, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include "ble/cs_Nordic.h"
#include "cfg/cs_Config.h"
#include "cfg/cs_Boards.h"
#include "drivers/cs_Serial.h"
#include "protocol/cs_CommandTypes.h"
#include "protocol/cs_ErrorCodes.h"
#include "structs/cs_PacketsInternal.h"
#include "util/cs_UuidParser.h"
#include <cstddef> // For NULL
#include <cstdint>
#include <type_traits>
#include <tuple>

#include <processing/behaviour/cs_Behaviour.h>

enum TypeBases {
	State_Base   = 0x000,
	General_Base = 0x100, // Configuration and state types are assumed to fit in a uint8_t, so lower than 256.
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
	CONFIG_DO_NOT_USE                       = State_Base,   // Record keys should be in the range 0x0001 - 0xBFFF. The value 0x0000 is reserved by the system. The values from 0xC000 to 0xFFFF are reserved for use by the Peer Manager module and can only be used in applications that do not include Peer Manager.
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
	CONFIG_ADV_INTERVAL                     = 12,     //  0x0C   // Advertising interval in units of 0.625ms.
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
	CONFIG_SPHERE_ID                        = 33,     //  0x21
	CONFIG_CROWNSTONE_ID                    = 34,     //  0x22
	CONFIG_KEY_ADMIN                        = 35,     //  0x23
	CONFIG_KEY_MEMBER                       = 36,     //  0x24
	CONFIG_KEY_BASIC                        = 37,     //  0x25
//	CONFIG_DEFAULT_ON                       = 38,     //  0x26
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
	CONFIG_KEY_SERVICE_DATA                 = 61,     //  0x3D
	CONFIG_MESH_DEVICE_KEY                  = 62,     //  0x3E
	CONFIG_MESH_APP_KEY                     = 63,     //  0x3F
	CONFIG_MESH_NET_KEY                     = 64,     //  0x40
	CONFIG_KEY_LOCALIZATION                 = 65,     //  0x41
	CONFIG_START_DIMMER_ON_ZERO_CROSSING    = 66,     //  0x42
	CONFIG_TAP_TO_TOGGLE_ENABLED            = 67,
	CONFIG_TAP_TO_TOGGLE_RSSI_THRESHOLD_OFFSET = 68,

	STATE_RESET_COUNTER                     = 128,    //  0x80 - 128
	STATE_SWITCH_STATE                      = 129,    //  0x81 - 129
	STATE_ACCUMULATED_ENERGY                = 130,    //  0x82 - 130   Energy used in μJ.
	STATE_POWER_USAGE                       = 131,    //  0x83 - 131   Power usage in mW.
//	STATE_TRACKED_DEVICES,                            //  0x84 - 132
	STATE_SCHEDULE                          = 133,    //  0x85 - 133
	STATE_OPERATION_MODE                    = 134,    //  0x86 - 134
	STATE_TEMPERATURE                       = 135,    //  0x87 - 135
	STATE_TIME                              = 136,    //  0x88 - 136
	STATE_FACTORY_RESET                     = 137,    //  0x89 - 137
//	STATE_LEARNED_SWITCHES,                           //  0x8A - 138
	STATE_ERRORS                            = 139,    //  0x8B - 139
//	STATE_ERROR_OVER_CURRENT,                         //  0x8C - 140
//	STATE_ERROR_OVER_CURRENT_DIMMER,                  //  0x8D - 141
//	STATE_ERROR_CHIP_TEMP,                            //  0x8E - 142
//	STATE_ERROR_DIMMER_TEMP,                          //  0x8F - 143
//	STATE_IGNORE_BITMASK,                             //  0x90 - 144
//	STATE_IGNORE_ALL,                                 //  0x91 - 145
//	STATE_IGNORE_LOCATION,                            //  0x92 - 146
//	STATE_ERROR_DIMMER_ON_FAILURE,                    //  0x93 - 147
//	STATE_ERROR_DIMMER_OFF_FAILURE,                   //  0x94 - 148
	STATE_SUN_TIME                          = 149,

	/*
	 * Internal commands and events.
	 * These don't need a specific order, as they're not exposed.
	 * Start at General_Base
	 */
	CMD_SWITCH_OFF = General_Base,                    // Sent to turn switch off.
	CMD_SWITCH_ON,                                    // Sent to turn switch on.
	CMD_SWITCH_TOGGLE,                                // Sent to toggle switch.
	CMD_SWITCH,                                       // Sent to set switch.
	CMD_MULTI_SWITCH,                                 // Sent to handle a multi switch. -- Payload is internal_multi_switch_item_cmd_t.
//	CMD_SET_LOG_LEVEL,
	CMD_ENABLE_LOG_POWER,                             // Sent to enable/disable power calculations logging. -- Payload is BOOL.
	CMD_ENABLE_LOG_CURRENT,                           // Sent to enable/disable current samples logging. -- Payload is BOOL.
	CMD_ENABLE_LOG_VOLTAGE,                           // Sent to enable/disable voltage samples logging. -- Payload is BOOL.
	CMD_ENABLE_LOG_FILTERED_CURRENT,                  // Sent to enable/disable filtered current samples logging. -- Payload is BOOL.
	CMD_RESET_DELAYED,                                // Sent to reboot. -- Payload is reset_delayed_t.
	CMD_ENABLE_ADVERTISEMENT,                         // Sent to enable/disable advertising. -- Payload is BOOL.
	CMD_ENABLE_MESH,                                  // Sent to enable/disable mesh. -- Payload is BOOL.
	CMD_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN,         // Sent to toggle ADC voltage pin. TODO: pin as payload?
	CMD_ENABLE_ADC_DIFFERENTIAL_CURRENT,              // Sent to toggle differential mode on current pin. -- Payload is BOOL.
	CMD_ENABLE_ADC_DIFFERENTIAL_VOLTAGE,              // Sent to toggle differential mode on voltage pin. -- Payload is BOOL.
	CMD_INC_VOLTAGE_RANGE,                            // Sent to increase voltage range.
	CMD_DEC_VOLTAGE_RANGE,                            // Sent to decrease voltage range.
	CMD_INC_CURRENT_RANGE,                            // Sent to increase current range.
	CMD_DEC_CURRENT_RANGE,                            // Sent to decrease current range.
	CMD_CONTROL_CMD,                                  // Sent to handle a control command. -- Payload is control_command_packet_t.
	CMD_SET_OPERATION_MODE,                           // Sent to switch operation mode. -- Payload is OperationMode.
	CMD_SEND_MESH_MSG,                                // Sent to send a mesh message. -- Payload is cs_mesh_msg_t.
	CMD_SEND_MESH_MSG_KEEP_ALIVE,                     // Sent to send a switch mesh message. -- Payload is keep_alive_state_item_t.
	CMD_SEND_MESH_MSG_MULTI_SWITCH,                   // Sent to send a switch mesh message. -- Payload is multi_switch_item_t.
	CMD_SET_TIME,                                     // Sent to set the time. -- Payload is uint32_t timestamp.
	CMD_FACTORY_RESET,                                // Sent when a factory reset should be performed: clear all data.
	EVT_TICK,                                         // Sent about every TICK_INTERVAL_MS ms. -- Payload is uint32_t counter.
	EVT_ADV_BACKGROUND,                               // Sent when a background advertisement has been received. -- Payload: adv_background_t.
	EVT_ADV_BACKGROUND_PARSED = 256+30,                        // Sent when a background advertisement has been validated and parsed. -- Payload: adv_background_parsed_t.
	EVT_ADVERTISEMENT_UPDATED,                        // Sent when advertisement was updated. TODO: advertisement data as payload?
	EVT_SCAN_STARTED,                                 // Sent when scanner started scanning.
	EVT_SCAN_STOPPED,                                 // Sent when scanner stopped scanning.
//	EVT_SCANNED_DEVICES,
	EVT_DEVICE_SCANNED,                               // Sent when a device was scanned. -- Payload is scanned_device_t.
//	EVT_POWER_SAMPLES_START,                          // Sent when the power samples buffer (for characteristic) is being filled with new data.
//	EVT_POWER_SAMPLES_END,                            // Sent when the power samples buffer (for characteristic) has been filled with new data.

	EVT_MESH_TIME,                                    // Sent when the mesh received the current time. -- Payload is uint32_t timestamp.
	EVT_SCHEDULE_ENTRIES_UPDATED,      // TODO: deprecate and use STATE event for this.                // Sent when schedule entries were changed. Payload is schedule_list_t.
//	EVT_BLE_EVENT,
	EVT_BLE_CONNECT,                                  // Sent when device connected.
	EVT_BLE_DISCONNECT,                               // Sent when device disconnected.
//	EVT_STATE_NOTIFICATION,            // Deprecated  // Sent when a state was updated.
	EVT_BROWNOUT_IMPENDING,                           // Sent when brownout is impending (low chip supply voltage)
	EVT_SESSION_NONCE_SET,                            // Sent when a session nonce is generated. -- Payload is the session nonce.
	EVT_KEEP_ALIVE,                                   // Sent when a keep alive witout action has been received.
	EVT_KEEP_ALIVE_STATE,                             // Sent when a keep alive with action has been received. -- Payload is keep_alive_state_item_cmd_t.
	EVT_CURRENT_USAGE_ABOVE_THRESHOLD,        // TODO: deprecate, use STATE_ERRORS        // Sent when current usage goes over the threshold.
	EVT_CURRENT_USAGE_ABOVE_THRESHOLD_DIMMER, // TODO: deprecate, use STATE_ERRORS        // Sent when current usage goes over the dimmer threshold, while dimmer is on.
	EVT_DIMMER_ON_FAILURE_DETECTED,           // TODO: deprecate, use STATE_ERRORS        // Sent when dimmer leaks current, while it's supposed to be off.
	EVT_DIMMER_OFF_FAILURE_DETECTED,          // TODO: deprecate, use STATE_ERRORS        // Sent when dimmer blocks current, while it's supposed to be on.
	EVT_CHIP_TEMP_ABOVE_THRESHOLD,            // TODO: deprecate, use STATE_ERRORS        // Sent when chip temperature is above threshold.
	EVT_CHIP_TEMP_OK,                         // TODO: deprecate, use STATE_ERRORS        // Sent when chip temperature is ok again.
	EVT_DIMMER_TEMP_ABOVE_THRESHOLD,          // TODO: deprecate, use STATE_ERRORS        // Sent when dimmer temperature is above threshold.
	EVT_DIMMER_TEMP_OK,                       // TODO: deprecate, use STATE_ERRORS        // Sent when dimmer temperature is ok again.
	EVT_DIMMER_FORCED_OFF,                            // Sent when dimmer was forced off.
	EVT_SWITCH_FORCED_OFF,                            // Sent when switch (relay and dimmer) was forced off.
	EVT_RELAY_FORCED_ON,                              // Sent when relay was forced on.

//	EVT_EXTERNAL_STATE_MSG_CHAN_0,     // Deprecated
//	EVT_EXTERNAL_STATE_MSG_CHAN_1,     // Deprecated
	EVT_TIME_SET,                                     // Sent when the time is set or changed. payload: previous posix time
	EVT_DIMMER_POWERED,                               // Sent when dimmer being powered is changed. -- Payload is BOOL, true when powered, and ready to be used.
	CMD_DIMMING_ALLOWED,	// Sent when commandhandler receives set request for allow dimming is changed. -- Payload is BOOL.
	CMD_SWITCH_LOCKED,		// Sent when commandhandler receives set request for lock state. -- Payload is BOOL.
	EVT_STORAGE_INITIALIZED,                          // Sent when Storage is initialized, storage is only usable after this event!
	EVT_STORAGE_WRITE_DONE,                           // Sent when an item has been written to storage. -- Payload is CS_TYPE, the type that was written.
	EVT_STORAGE_REMOVE_DONE,                          // Sent when an item has been invalidated at storage. -- Payload is CS_TYPE, the type that was invalidated.
	EVT_STORAGE_REMOVE_FILE_DONE,                     // Sent when a file has been invalidated at storage. -- Payload is cs_file_id_t, the file that was invalidated.
	EVT_STORAGE_GC_DONE,                              // Sent when garbage collection is done, invalidated data is actually removed at this point.
	EVT_STORAGE_FACTORY_RESET,                        // Sent when factory reset of storage is done.
	EVT_STORAGE_PAGES_ERASED,                         // Sent when all storage pages are completely erased.
	EVT_MESH_FACTORY_RESET,                           // Sent when factory reset of mesh storage is done.
	EVT_SETUP_DONE,                                   // Sent when setup was done (and settings are stored).
//	EVT_DO_RESET_DELAYED,                             // Sent to perform a reset in a few seconds.
//	EVT_STORAGE_WRITE,                                // Sent when an item is going to be written to storage.
//	EVT_STORAGE_ERASE,                                // Sent when a flash page is going to be erased.
	EVT_ADC_RESTARTED,                                // Sent when ADC has been restarted.
	EVT_STATE_EXTERNAL_STONE,                          // Sent when the state of another stone has been received. -- Payload is state_external_stone_t

	// ------------------------
	EVT_SAVE_BEHAVIOUR, 						// when a user requests to save a behaviour, this event fires.
	EVT_REPLACE_BEHAVIOUR,						// when a user requests to update a behaviour, this event fires.
	EVT_REMOVE_BEHAVIOUR,						// when a user requests to remove a behaviour, this event fires.
	EVT_GET_BEHAVIOUR,							// when a user requests a currently active behaviour, this event fires.
	EVT_GET_BEHAVIOUR_INDICES,                  // Sent when a user requests a list of indices with active behaviours.
	EVT_BEHAVIOURSTORE_MUTATION,				// Sent by BehaviourStore for other components to react _after_ a change to the behaviourstore occured.
	EVT_PRESENCE_MUTATION,						// when a change in presence occurs this event fires.
	EVT_BEHAVIOUR_SWITCH_STATE,					// when behaviour desires a stateswitch this event is fired.
	CMD_SET_RELAY,								// when a user requests to set the relay to a specific state
	CMD_SET_DIMMER,								// when a user requests to set the dimmer to a specific state
	// ------------------------
	//
	EVT_PROFILE_LOCATION,                       // profile and location information 
};

CS_TYPE toCsType(uint16_t type);

/*---------------------------------------------------------------------------------------------------------------------
 *
 *                                               Custom structs
 *
 *-------------------------------------------------------------------------------------------------------------------*/

// TODO: these definitions (also the structs) should be moved to somewhere else.

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

typedef uint16_t cs_file_id_t;

static const cs_file_id_t FILE_DO_NOT_USE     = 0x0000;
static const cs_file_id_t FILE_KEEP_FOREVER   = 0x0001;
static const cs_file_id_t FILE_CONFIGURATION  = 0x0003;



/*---------------------------------------------------------------------------------------------------------------------
 *
 *                                               Types
 *
 *-------------------------------------------------------------------------------------------------------------------*/

#ifndef TYPIFY
#define TYPIFY(NAME) NAME ## _TYPE
#endif
typedef uint16_t TYPIFY(CONFIG_ADV_INTERVAL);
typedef uint16_t TYPIFY(CONFIG_BOOT_DELAY);
typedef uint8_t  TYPIFY(CONFIG_SPHERE_ID);
typedef uint8_t  TYPIFY(CONFIG_CROWNSTONE_ID);
typedef    float TYPIFY(CONFIG_CURRENT_MULTIPLIER);
typedef  int32_t TYPIFY(CONFIG_CURRENT_ADC_ZERO);
typedef     BOOL TYPIFY(CONFIG_ENCRYPTION_ENABLED);
typedef     BOOL TYPIFY(CONFIG_IBEACON_ENABLED);
typedef uint16_t TYPIFY(CONFIG_IBEACON_MINOR);
typedef uint16_t TYPIFY(CONFIG_IBEACON_MAJOR);
typedef   int8_t TYPIFY(CONFIG_IBEACON_TXPOWER);
typedef   int8_t TYPIFY(CONFIG_LOW_TX_POWER);
typedef   int8_t TYPIFY(CONFIG_MAX_CHIP_TEMP);
typedef     BOOL TYPIFY(CONFIG_MESH_ENABLED);
typedef  int32_t TYPIFY(CONFIG_POWER_ZERO);
typedef uint32_t TYPIFY(CONFIG_PWM_PERIOD);
typedef    float TYPIFY(CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP);
typedef    float TYPIFY(CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN);
typedef     BOOL TYPIFY(CONFIG_PWM_ALLOWED);
typedef uint16_t TYPIFY(CONFIG_RELAY_HIGH_DURATION);
typedef     BOOL TYPIFY(CONFIG_SCANNER_ENABLED);
typedef uint16_t TYPIFY(CONFIG_SCAN_BREAK_DURATION);
typedef uint16_t TYPIFY(CONFIG_SCAN_DURATION);
typedef uint16_t TYPIFY(CONFIG_SCAN_INTERVAL);
typedef uint16_t TYPIFY(CONFIG_SCAN_WINDOW);
typedef uint16_t TYPIFY(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD);
typedef uint16_t TYPIFY(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM);
typedef     BOOL TYPIFY(CONFIG_START_DIMMER_ON_ZERO_CROSSING);
typedef     BOOL TYPIFY(CONFIG_SWITCH_LOCKED);
typedef     BOOL TYPIFY(CONFIG_SWITCHCRAFT_ENABLED);
typedef    float TYPIFY(CONFIG_SWITCHCRAFT_THRESHOLD);
typedef     BOOL TYPIFY(CONFIG_TAP_TO_TOGGLE_ENABLED);
typedef   int8_t TYPIFY(CONFIG_TAP_TO_TOGGLE_RSSI_THRESHOLD_OFFSET);
typedef   int8_t TYPIFY(CONFIG_TX_POWER);
typedef  uint8_t TYPIFY(CONFIG_UART_ENABLED); //TODO: serial_enable_t
typedef    float TYPIFY(CONFIG_VOLTAGE_MULTIPLIER);
typedef  int32_t TYPIFY(CONFIG_VOLTAGE_ADC_ZERO);

typedef  int64_t TYPIFY(STATE_ACCUMULATED_ENERGY);
typedef state_errors_t TYPIFY(STATE_ERRORS);
typedef  uint8_t TYPIFY(STATE_FACTORY_RESET);
typedef  uint8_t TYPIFY(STATE_OPERATION_MODE);
typedef  int32_t TYPIFY(STATE_POWER_USAGE);
typedef uint16_t TYPIFY(STATE_RESET_COUNTER);
typedef schedule_list_t TYPIFY(STATE_SCHEDULE);
typedef switch_state_t TYPIFY(STATE_SWITCH_STATE);
typedef   int8_t TYPIFY(STATE_TEMPERATURE);
typedef uint32_t TYPIFY(STATE_TIME);
typedef set_sun_time_t TYPIFY(STATE_SUN_TIME);

typedef  void TYPIFY(EVT_ADC_RESTARTED);
typedef  adv_background_t TYPIFY(EVT_ADV_BACKGROUND);
typedef  adv_background_parsed_t TYPIFY(EVT_ADV_BACKGROUND_PARSED);
typedef  void TYPIFY(EVT_ADVERTISEMENT_UPDATED);
typedef  void TYPIFY(EVT_BLE_CONNECT);
typedef  void TYPIFY(EVT_BLE_DISCONNECT);
typedef  void TYPIFY(EVT_BROWNOUT_IMPENDING);
typedef  void TYPIFY(EVT_CHIP_TEMP_ABOVE_THRESHOLD);
typedef  void TYPIFY(EVT_CHIP_TEMP_OK);
typedef  reset_delayed_t TYPIFY(CMD_RESET_DELAYED);
typedef  OperationMode TYPIFY(CMD_SET_OPERATION_MODE);
typedef  void TYPIFY(EVT_CURRENT_USAGE_ABOVE_THRESHOLD_DIMMER);
typedef  void TYPIFY(EVT_CURRENT_USAGE_ABOVE_THRESHOLD);
typedef  void TYPIFY(CMD_DEC_CURRENT_RANGE);
typedef  void TYPIFY(CMD_DEC_VOLTAGE_RANGE);
typedef  scanned_device_t TYPIFY(EVT_DEVICE_SCANNED);
typedef  void TYPIFY(EVT_DIMMER_ON_FAILURE_DETECTED);
typedef  void TYPIFY(EVT_DIMMER_OFF_FAILURE_DETECTED);
typedef  BOOL TYPIFY(CMD_ENABLE_ADC_DIFFERENTIAL_CURRENT);
typedef  BOOL TYPIFY(CMD_ENABLE_ADC_DIFFERENTIAL_VOLTAGE);
typedef  BOOL TYPIFY(CMD_ENABLE_ADVERTISEMENT);
typedef  BOOL TYPIFY(CMD_ENABLE_LOG_CURRENT);
typedef  BOOL TYPIFY(CMD_ENABLE_LOG_FILTERED_CURRENT);
typedef  BOOL TYPIFY(CMD_ENABLE_LOG_POWER);
typedef  BOOL TYPIFY(CMD_ENABLE_LOG_VOLTAGE);
typedef  BOOL TYPIFY(CMD_ENABLE_MESH);
typedef  void TYPIFY(CMD_INC_VOLTAGE_RANGE);
typedef  void TYPIFY(CMD_INC_CURRENT_RANGE);
typedef  void TYPIFY(EVT_KEEP_ALIVE);
typedef  keep_alive_state_item_cmd_t TYPIFY(EVT_KEEP_ALIVE_STATE);
typedef  uint32_t TYPIFY(EVT_MESH_TIME);
typedef  void TYPIFY(CMD_SWITCH_OFF);
typedef  void TYPIFY(CMD_SWITCH_ON);
typedef  void TYPIFY(CMD_SWITCH_TOGGLE);
typedef  internal_multi_switch_item_cmd_t TYPIFY(CMD_SWITCH);
typedef  internal_multi_switch_item_t TYPIFY(CMD_MULTI_SWITCH);
typedef  cs_mesh_msg_t TYPIFY(CMD_SEND_MESH_MSG);
typedef  keep_alive_state_item_t TYPIFY(CMD_SEND_MESH_MSG_KEEP_ALIVE);
typedef  internal_multi_switch_item_t TYPIFY(CMD_SEND_MESH_MSG_MULTI_SWITCH);
typedef  uint32_t TYPIFY(CMD_SET_TIME);
typedef  void TYPIFY(CMD_FACTORY_RESET);
typedef  BOOL TYPIFY(CMD_DIMMING_ALLOWED);
typedef  void TYPIFY(EVT_DIMMER_FORCED_OFF);
typedef  BOOL TYPIFY(EVT_DIMMER_POWERED);
typedef  void TYPIFY(EVT_DIMMER_TEMP_ABOVE_THRESHOLD);
typedef  void TYPIFY(EVT_DIMMER_TEMP_OK);
typedef  void TYPIFY(EVT_RELAY_FORCED_ON);
typedef  control_command_packet_t TYPIFY(CMD_CONTROL_CMD);
typedef  void TYPIFY(EVT_SCAN_STARTED);
typedef  void TYPIFY(EVT_SCAN_STOPPED);
typedef  schedule_list_t TYPIFY(EVT_SCHEDULE_ENTRIES_UPDATED);
typedef  void TYPIFY(EVT_SETUP_DONE);
typedef  session_nonce_t TYPIFY(EVT_SESSION_NONCE_SET);
typedef  state_external_stone_t TYPIFY(EVT_STATE_EXTERNAL_STONE);
typedef  void TYPIFY(EVT_STORAGE_INITIALIZED);
typedef  CS_TYPE TYPIFY(EVT_STORAGE_WRITE_DONE);
typedef  CS_TYPE TYPIFY(EVT_STORAGE_REMOVE_DONE);
typedef  cs_file_id_t TYPIFY(EVT_STORAGE_REMOVE_FILE_DONE);
typedef  void TYPIFY(EVT_STORAGE_GC_DONE);
typedef  void TYPIFY(EVT_STORAGE_FACTORY_RESET);
typedef  void TYPIFY(EVT_STORAGE_PAGES_ERASED);
typedef  void TYPIFY(EVT_MESH_FACTORY_RESET);
typedef  void TYPIFY(EVT_SWITCH_FORCED_OFF);
typedef  BOOL TYPIFY(CMD_SWITCH_LOCKED);
typedef  uint32_t TYPIFY(EVT_TICK);
typedef  uint32_t TYPIFY(EVT_TIME_SET);
typedef  void TYPIFY(CMD_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN);
typedef Behaviour TYPIFY(EVT_SAVE_BEHAVIOUR);
typedef std::tuple<uint8_t,Behaviour> TYPIFY(EVT_REPLACE_BEHAVIOUR);
typedef uint8_t TYPIFY(EVT_REMOVE_BEHAVIOUR); // index
typedef uint8_t TYPIFY(EVT_GET_BEHAVIOUR); // index
typedef void TYPIFY(EVT_GET_BEHAVIOUR_INDICES);
typedef void TYPIFY(EVT_BEHAVIOURSTORE_MUTATION);
typedef uint8_t TYPIFY(EVT_BEHAVIOUR_SWITCH_STATE);
typedef void TYPIFY(EVT_PRESENCE_MUTATION);
typedef bool TYPIFY(CMD_SET_RELAY);
typedef uint8_t TYPIFY(CMD_SET_DIMMER); // interpret as intensity value, not combined with relay state.
typedef cs_mesh_model_msg_profile_location_t TYPIFY(EVT_PROFILE_LOCATION);

/*---------------------------------------------------------------------------------------------------------------------
 *
 *                                               Sizes
 *
 *-------------------------------------------------------------------------------------------------------------------*/



/**
 * The size of a particular default value. In case of strings or arrays this is the maximum size of the corresponding
 * field. There are no fields that are of unrestricted size. For fields that are not implemented it is possible to
 * set size to 0.
 */
size16_t TypeSize(CS_TYPE const & type);

/*---------------------------------------------------------------------------------------------------------------------
 *
 *                                               Names
 *
 *-------------------------------------------------------------------------------------------------------------------*/


const char* TypeName(CS_TYPE const & type);

/**
 * Get the storage file id of a given a type.
 */
cs_file_id_t getFileId(CS_TYPE const & type);


/*---------------------------------------------------------------------------------------------------------------------
 *
 *                                               Defaults
 *
 *-------------------------------------------------------------------------------------------------------------------*/



/**
 * Gives the required access level to set a state type.
 */
EncryptionAccessLevel getUserAccessLevelSet(CS_TYPE const & type);

/**
 * Gives the required access level to get a state type.
 */
EncryptionAccessLevel getUserAccessLevelGet(CS_TYPE const & type);
