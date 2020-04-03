/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 23, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include "cfg/cs_Config.h"
#include "drivers/cs_Serial.h"
#include "protocol/cs_CommandTypes.h"
#include "protocol/cs_ErrorCodes.h"
#include "structs/cs_PacketsInternal.h"
#include <cstddef> // For NULL
#include <cstdint>
#include <type_traits>
#include <tuple>

#include <behaviour/cs_SwitchBehaviour.h>
#include <behaviour/cs_TwilightBehaviour.h>
#include <behaviour/cs_ExtendedSwitchBehaviour.h>

// #include <presence/cs_PresenceHandler.h>

enum TypeBases {
	State_Base   = 0x000,
	Internal_Base = 0x100, // Mesh state set relies on state types being below 0x100.
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

/**
 * Types:
 * - State types:
 *   - Prefixed with STATE (or old style: CONFIG)
 *   - These types can be stored in the State class.
 *   - Should have a fixed number, smaller than General_Base.
 * - Event types:
 *   - Prefixed with EVT.
 *   - Types that are sent to inform when something happened, or is going to happen.
 *   - If a fixed number is required, put it right after Internal_Base.
 * - Command types:
 *   - Prefixed with CMD.
 *   - Types that are sent to request something to be done.
 *   - If a fixed number is required, put it right after Internal_Base.
 */
enum class CS_TYPE: uint16_t {
	CONFIG_DO_NOT_USE                       = State_Base,   // Record keys should be in the range 0x0001 - 0xBFFF. The value 0x0000 is reserved by the system. The values from 0xC000 to 0xFFFF are reserved for use by the Peer Manager module and can only be used in applications that do not include Peer Manager.
//	CONFIG_DEVICE_TYPE                      = 1,
//	CONFIG_ROOM                             = 2,
//	CONFIG_FLOOR                            = 3,
//	CONFIG_NEARBY_TIMEOUT                   = 4,
	CONFIG_PWM_PERIOD                       = 5,
	CONFIG_IBEACON_MAJOR                    = 6,
	CONFIG_IBEACON_MINOR                    = 7,
	CONFIG_IBEACON_UUID                     = 8,
	CONFIG_IBEACON_TXPOWER                  = 9,
//	CONFIG_WIFI_SETTINGS                    = 10,
	CONFIG_TX_POWER                         = 11,
	CONFIG_ADV_INTERVAL                     = 12,  // Advertising interval in units of 0.625ms.
//	CONFIG_PASSKEY                          = 13,
//	CONFIG_MIN_ENV_TEMP                     = 14,
//	CONFIG_MAX_ENV_TEMP                     = 15,
	CONFIG_SCAN_DURATION                    = 16,  // Deprecate
//	CONFIG_SCAN_SEND_DELAY                  = 17,
	CONFIG_SCAN_BREAK_DURATION              = 18,  // Deprecate
	CONFIG_BOOT_DELAY                       = 19,
	CONFIG_MAX_CHIP_TEMP                    = 20,
//	CONFIG_SCAN_FILTER                      = 21,
//	CONFIG_SCAN_FILTER_SEND_FRACTION        = 22,
	CONFIG_CURRENT_LIMIT                    = 23,  // Not implemented yet, but something we want in the future.
	CONFIG_MESH_ENABLED                     = 24,
	CONFIG_ENCRYPTION_ENABLED               = 25,
	CONFIG_IBEACON_ENABLED                  = 26,
	CONFIG_SCANNER_ENABLED                  = 27,
//	CONFIG_CONT_POWER_SAMPLER_ENABLED       = 28,
//	CONFIG_TRACKER_ENABLED                  = 29,
//	CONFIG_ADC_BURST_SAMPLE_RATE            = 30,
//	CONFIG_POWER_SAMPLE_BURST_INTERVAL      = 31,
//	CONFIG_POWER_SAMPLE_CONT_INTERVAL       = 32,
	CONFIG_SPHERE_ID                        = 33,
	CONFIG_CROWNSTONE_ID                    = 34,
	CONFIG_KEY_ADMIN                        = 35,
	CONFIG_KEY_MEMBER                       = 36,
	CONFIG_KEY_BASIC                        = 37,
//	CONFIG_DEFAULT_ON                       = 38,
	CONFIG_SCAN_INTERVAL                    = 39,
	CONFIG_SCAN_WINDOW                      = 40,
	CONFIG_RELAY_HIGH_DURATION              = 41,
	CONFIG_LOW_TX_POWER                     = 42,
	CONFIG_VOLTAGE_MULTIPLIER               = 43,
	CONFIG_CURRENT_MULTIPLIER               = 44,
	CONFIG_VOLTAGE_ADC_ZERO                 = 45,
	CONFIG_CURRENT_ADC_ZERO                 = 46,
	CONFIG_POWER_ZERO                       = 47,
//	CONFIG_POWER_ZERO_AVG_WINDOW            = 48,
//	CONFIG_MESH_ACCESS_ADDRESS              = 49,
	CONFIG_SOFT_FUSE_CURRENT_THRESHOLD      = 50,
	CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_PWM  = 51,
	CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP    = 52,
	CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN  = 53,
	CONFIG_PWM_ALLOWED                      = 54,
	CONFIG_SWITCH_LOCKED                    = 55,
	CONFIG_SWITCHCRAFT_ENABLED              = 56,
	CONFIG_SWITCHCRAFT_THRESHOLD            = 57,
//	CONFIG_MESH_CHANNEL                     = 58,
	CONFIG_UART_ENABLED                     = 59,
	CONFIG_NAME                             = 60,
	CONFIG_KEY_SERVICE_DATA                 = 61,
	CONFIG_MESH_DEVICE_KEY                  = 62,
	CONFIG_MESH_APP_KEY                     = 63,
	CONFIG_MESH_NET_KEY                     = 64,
	CONFIG_KEY_LOCALIZATION                 = 65,
	CONFIG_START_DIMMER_ON_ZERO_CROSSING    = 66,
	CONFIG_TAP_TO_TOGGLE_ENABLED            = 67,
	CONFIG_TAP_TO_TOGGLE_RSSI_THRESHOLD_OFFSET = 68,
	STATE_BEHAVIOUR_RULE                    = 69,
	STATE_TWILIGHT_RULE                     = 70,
	STATE_EXTENDED_BEHAVIOUR_RULE			= 71,

	STATE_RESET_COUNTER                     = 128,
	STATE_SWITCH_STATE                      = 129,
	STATE_ACCUMULATED_ENERGY                = 130,    // Energy used in μJ.
	STATE_POWER_USAGE                       = 131,    // Power usage in mW.
//	STATE_TRACKED_DEVICES,
//	STATE_SCHEDULE                          = 133,
	STATE_OPERATION_MODE                    = 134,
	STATE_TEMPERATURE                       = 135,
	STATE_TIME                              = 136,
	STATE_FACTORY_RESET                     = 137,
//	STATE_LEARNED_SWITCHES,
	STATE_ERRORS                            = 139,
//	STATE_ERROR_OVER_CURRENT,
//	STATE_ERROR_OVER_CURRENT_DIMMER,
//	STATE_ERROR_CHIP_TEMP,
//	STATE_ERROR_DIMMER_TEMP,
//	STATE_IGNORE_BITMASK,
//	STATE_IGNORE_ALL,
//	STATE_IGNORE_LOCATION,
//	STATE_ERROR_DIMMER_ON_FAILURE,
//	STATE_ERROR_DIMMER_OFF_FAILURE,
	STATE_SUN_TIME                          = 149,
	STATE_BEHAVIOUR_SETTINGS                = 150,
	STATE_MESH_IV_INDEX                     = 151,
	STATE_MESH_SEQ_NUMBER                   = 152,
	STATE_BEHAVIOUR_MASTER_HASH             = 153,

	/*
	 * Internal commands and events.
	 * Start at Internal_Base.
	 */
	// Types with fixed number.
	EVT_ADV_BACKGROUND_PARSED = Internal_Base + 0,        // Sent when a background advertisement has been validated and parsed.

	// Scans
	EVT_DEVICE_SCANNED= Internal_Base + 1,                               // Device was scanned.
	EVT_ADV_BACKGROUND= Internal_Base + 2,                               // Background advertisement has been received.
	EVT_ADV_BACKGROUND_PARSED_V1= Internal_Base + 3,                     // Sent when a v1 background advertisement has been received.

	// Switch
	CMD_SWITCH_OFF= Internal_Base + 4,                                   // Turn switch off.
	CMD_SWITCH_ON= Internal_Base + 5,                                    // Turn switch on.
	CMD_SWITCH_TOGGLE= Internal_Base + 6,                                // Toggle switch.
	CMD_SWITCH= Internal_Base + 7,                                       // Set switch.
	CMD_SET_RELAY= Internal_Base + 8,                                    // Set the relay state.
	CMD_SET_DIMMER= Internal_Base + 9,                                   // Set the dimmer state.
	CMD_MULTI_SWITCH= Internal_Base + 10,                                 // Handle a multi switch.
	CMD_SWITCHING_ALLOWED= Internal_Base + 11,		                      // Set switch lock.
	CMD_DIMMING_ALLOWED= Internal_Base + 12,	                          // Set allow dimming.
	EVT_DIMMER_POWERED= Internal_Base + 13,                               // Dimmer being powered is changed. Payload: true when powered, and ready to be used.

	// Errors
	EVT_CURRENT_USAGE_ABOVE_THRESHOLD= Internal_Base + 14,                // Current usage goes over the threshold.
	EVT_CURRENT_USAGE_ABOVE_THRESHOLD_DIMMER= Internal_Base + 15,         // Current usage goes over the dimmer threshold, while dimmer is on.
	EVT_DIMMER_ON_FAILURE_DETECTED= Internal_Base + 16,                   // Dimmer leaks current, while it's supposed to be off.
	EVT_DIMMER_OFF_FAILURE_DETECTED= Internal_Base + 17,                  // Dimmer blocks current, while it's supposed to be on.
	EVT_CHIP_TEMP_ABOVE_THRESHOLD= Internal_Base + 18,                    // Chip temperature is above threshold.
	EVT_CHIP_TEMP_OK= Internal_Base + 19,                                 // Chip temperature is ok again.
	EVT_DIMMER_TEMP_ABOVE_THRESHOLD= Internal_Base + 20,                  // Dimmer temperature is above threshold.
	EVT_DIMMER_TEMP_OK= Internal_Base + 21,                               // Dimmer temperature is ok again.
	EVT_DIMMER_FORCED_OFF= Internal_Base + 22,                            // Dimmer was forced off.
	EVT_SWITCH_FORCED_OFF= Internal_Base + 23,                            // Switch (relay and dimmer) was forced off.
	EVT_RELAY_FORCED_ON= Internal_Base + 24,                              // Relay was forced on.

	// Storage
	EVT_STORAGE_INITIALIZED= Internal_Base + 25,                          // Storage is initialized, storage is only usable after this event!
	EVT_STORAGE_WRITE_DONE= Internal_Base + 26,                           // An item has been written to storage.
	EVT_STORAGE_REMOVE_DONE= Internal_Base + 27,                          // An item has been invalidated at storage.
	EVT_STORAGE_REMOVE_ALL_TYPES_WITH_ID_DONE= Internal_Base + 28,        // All state values with a certain ID have been invalidated at storage.
	EVT_STORAGE_GC_DONE= Internal_Base + 29,                              // Garbage collection is done, invalidated data is actually removed at this point.
	EVT_STORAGE_FACTORY_RESET_DONE= Internal_Base + 30,                   // Factory reset of storage is done. /!\ Only to be used by State.
	EVT_STORAGE_PAGES_ERASED= Internal_Base + 31,                         // All storage pages are completely erased.
	CMD_FACTORY_RESET= Internal_Base + 32,                                // Perform a factory reset: clear all data.
	EVT_STATE_FACTORY_RESET_DONE= Internal_Base + 33,                     // Factory reset of state is done.
	EVT_MESH_FACTORY_RESET_DONE= Internal_Base + 34,                           // Factory reset of mesh storage is done.

	// Logging
	CMD_ENABLE_LOG_POWER= Internal_Base + 35,                             // Enable/disable power calculations logging.
	CMD_ENABLE_LOG_CURRENT= Internal_Base + 36,                           // Enable/disable current samples logging.
	CMD_ENABLE_LOG_VOLTAGE= Internal_Base + 37,                           // Enable/disable voltage samples logging.
	CMD_ENABLE_LOG_FILTERED_CURRENT= Internal_Base + 38,                  // Enable/disable filtered current samples logging.

	// ADC config
	CMD_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN= Internal_Base + 39,         // Toggle ADC voltage pin. TODO: pin as payload?
	CMD_ENABLE_ADC_DIFFERENTIAL_CURRENT= Internal_Base + 40,              // Toggle differential mode on current pin.
	CMD_ENABLE_ADC_DIFFERENTIAL_VOLTAGE= Internal_Base + 41,              // Toggle differential mode on voltage pin.
	CMD_INC_VOLTAGE_RANGE= Internal_Base + 42,                            // Increase voltage range.
	CMD_DEC_VOLTAGE_RANGE= Internal_Base + 43,                            // Decrease voltage range.
	CMD_INC_CURRENT_RANGE= Internal_Base + 44,                            // Increase current range.
	CMD_DEC_CURRENT_RANGE= Internal_Base + 45,                            // Decrease current range.

	// Mesh

	CMD_SEND_MESH_MSG,                                // Send a mesh message.
	CMD_SEND_MESH_MSG_SET_TIME,                       // Send a set time mesh message.
	CMD_SEND_MESH_MSG_NOOP,                           // Send a noop mesh message.
	CMD_SEND_MESH_MSG_MULTI_SWITCH,                   // Send a switch mesh message.
	CMD_SEND_MESH_MSG_PROFILE_LOCATION,               // Send a profile location mesh message.
	CMD_SEND_MESH_MSG_SET_BEHAVIOUR_SETTINGS,         // Send a set behaviour settings mesh message.
	CMD_SEND_MESH_MSG_TRACKED_DEVICE_REGISTER,
	CMD_SEND_MESH_MSG_TRACKED_DEVICE_TOKEN,
	CMD_SEND_MESH_MSG_TRACKED_DEVICE_LIST_SIZE,
	CMD_SEND_MESH_CONTROL_COMMAND,                    // Send a control command via the mesh. All permission checks must have been done already!
	EVT_MESH_TIME,                                    // Mesh received the current time.
	EVT_MESH_TRACKED_DEVICE_REGISTER,                 // Mesh received a tracked device to register.
	EVT_MESH_TRACKED_DEVICE_TOKEN,                    // Mesh received a tracked device token.
	EVT_MESH_TRACKED_DEVICE_LIST_SIZE,                // Mesh received a tracked device list size.
	EVT_MESH_SYNC_REQUEST_OUTGOING,                   // Before an outgoing sync request is broadcasted, this event is fired internally so that other event handlers can tag on.
	EVT_MESH_SYNC_REQUEST_INCOMING,                   // When a sync request is received, this event is fired internally so that each event handler can individually respond to it.
	EVT_MESH_SYNC_FAILED,                             // When syncing is considered to have failed, no more retries.
	EVT_MESH_EXT_STATE_0,                             // Mesh received part 0 of the state of a Crownstone.
	EVT_MESH_EXT_STATE_1,                             // Mesh received part 1 of the state of a Crownstone.
	EVT_MESH_PAGES_ERASED,                            // All mesh storage pages are completely erased.

	// Behaviour
	CMD_ADD_BEHAVIOUR= Internal_Base + 61,                                // Add a behaviour.
	CMD_REPLACE_BEHAVIOUR= Internal_Base + 62,                            // Replace a behaviour.
	CMD_REMOVE_BEHAVIOUR= Internal_Base + 63,                             // Remove a behaviour.
	CMD_GET_BEHAVIOUR= Internal_Base + 64,                                // Get a behaviour.
	CMD_GET_BEHAVIOUR_INDICES= Internal_Base + 65,                        // Get a list of indices of active behaviours.
	CMD_GET_BEHAVIOUR_DEBUG= Internal_Base + 66,                          // Get info to debug behaviour. Multiple classes will handle this command to fill pieces of info.
	EVT_BEHAVIOURSTORE_MUTATION= Internal_Base + 67,                      // Sent by BehaviourStore, after a change to the stored behaviours.
	EVT_BEHAVIOUR_OVERRIDDEN= Internal_Base + 68,                         // Informs whether behaviour is overridden by user (in override state).

	// Tracked devices
	CMD_REGISTER_TRACKED_DEVICE= Internal_Base + 69,
	CMD_UPDATE_TRACKED_DEVICE= Internal_Base + 70,

	EVT_PROFILE_LOCATION= Internal_Base + 71,                             // Location of profile.

	CMD_RESET_DELAYED= Internal_Base + 72,                                // Reboot.
	EVT_GOING_TO_DFU= Internal_Base + 73,                                 // The system will reboot to DFU mode soon.

	CMD_ENABLE_ADVERTISEMENT= Internal_Base + 74,                         // Enable/disable advertising.
	CMD_ENABLE_MESH= Internal_Base + 75,                                  // Enable/disable mesh.

	CMD_SET_TIME= Internal_Base + 76,                                     // Set the time.
	EVT_TIME_SET= Internal_Base + 77,                                     // Time is set or changed. Payload: previous posix time

	CMD_CONTROL_CMD= Internal_Base + 78,                                  // Handle a control command.

	EVT_TICK= Internal_Base + 79,                                         // Sent about every TICK_INTERVAL_MS ms.

	EVT_STATE_EXTERNAL_STONE= Internal_Base + 80,                         // The state of another stone has been received.

	EVT_ADVERTISEMENT_UPDATED= Internal_Base + 81,                        // Advertisement was updated. TODO: advertisement data as payload?

	EVT_SCAN_STARTED= Internal_Base + 82,                                 // Scanner started scanning.
	EVT_SCAN_STOPPED= Internal_Base + 83,                                 // Scanner stopped scanning.

	EVT_BLE_CONNECT= Internal_Base + 84,                                  // Device connected.
	EVT_BLE_DISCONNECT= Internal_Base + 85,                               // Device disconnected.

	EVT_BROWNOUT_IMPENDING= Internal_Base + 86,                           // Brownout is impending (low chip supply voltage).

	EVT_SESSION_NONCE_SET= Internal_Base + 87,                            // Session nonce was generated.

	EVT_SETUP_DONE= Internal_Base + 88,                                   // Setup is done (and settings are stored).

	EVT_ADC_RESTARTED= Internal_Base + 89,                                // ADC has been restarted.

	EVT_PRESENCE_MUTATION= Internal_Base + 90,                            // Presence changed.


	EVT_GENERIC_TEST= 0xFFFF,                         // Can be used by the python test python lib for ad hoc tests during development.

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

struct __attribute__((packed)) cs_type_and_id_t {
	CS_TYPE type;
	cs_state_id_t id;
};


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
typedef uint16_t TYPIFY(CONFIG_IBEACON_MAJOR);
typedef uint16_t TYPIFY(CONFIG_IBEACON_MINOR);
typedef cs_uuid128_t TYPIFY(CONFIG_IBEACON_UUID);
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
typedef switch_state_t TYPIFY(STATE_SWITCH_STATE);
typedef   int8_t TYPIFY(STATE_TEMPERATURE);
typedef uint32_t TYPIFY(STATE_TIME);
typedef sun_time_t TYPIFY(STATE_SUN_TIME);
typedef void TYPIFY(STATE_BEHAVIOUR_RULE);
typedef void TYPIFY(STATE_TWILIGHT_RULE);
typedef void TYPIFY(STATE_EXTENDED_BEHAVIOUR_RULE);
typedef behaviour_settings_t TYPIFY(STATE_BEHAVIOUR_SETTINGS);
typedef uint32_t TYPIFY(STATE_BEHAVIOUR_MASTER_HASH);
typedef cs_mesh_iv_index_t TYPIFY(STATE_MESH_IV_INDEX);
typedef cs_mesh_seq_number_t TYPIFY(STATE_MESH_SEQ_NUMBER);

typedef  void TYPIFY(EVT_ADC_RESTARTED);
typedef  adv_background_t TYPIFY(EVT_ADV_BACKGROUND);
typedef  adv_background_parsed_t TYPIFY(EVT_ADV_BACKGROUND_PARSED);
typedef  adv_background_parsed_v1_t TYPIFY(EVT_ADV_BACKGROUND_PARSED_V1);
typedef  void TYPIFY(EVT_ADVERTISEMENT_UPDATED);
typedef  void TYPIFY(EVT_BLE_CONNECT);
typedef  void TYPIFY(EVT_BLE_DISCONNECT);
typedef  void TYPIFY(EVT_BROWNOUT_IMPENDING);
typedef  void TYPIFY(EVT_CHIP_TEMP_ABOVE_THRESHOLD);
typedef  void TYPIFY(EVT_CHIP_TEMP_OK);
typedef  reset_delayed_t TYPIFY(CMD_RESET_DELAYED);
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
typedef  uint32_t TYPIFY(EVT_MESH_TIME);
typedef  cs_mesh_model_msg_device_register_t TYPIFY(EVT_MESH_TRACKED_DEVICE_REGISTER);
typedef  cs_mesh_model_msg_device_register_t TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_REGISTER);
typedef  cs_mesh_model_msg_device_token_t TYPIFY(EVT_MESH_TRACKED_DEVICE_TOKEN);
typedef  cs_mesh_model_msg_device_token_t TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_TOKEN);
typedef  cs_mesh_model_msg_device_list_size_t TYPIFY(EVT_MESH_TRACKED_DEVICE_LIST_SIZE);
typedef  cs_mesh_model_msg_device_list_size_t TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_LIST_SIZE);
typedef  mesh_control_command_packet_t TYPIFY(CMD_SEND_MESH_CONTROL_COMMAND);
typedef  void TYPIFY(CMD_SWITCH_OFF);
typedef  void TYPIFY(CMD_SWITCH_ON);
typedef  void TYPIFY(CMD_SWITCH_TOGGLE);
typedef  internal_multi_switch_item_cmd_t TYPIFY(CMD_SWITCH);
typedef  internal_multi_switch_item_t TYPIFY(CMD_MULTI_SWITCH);
typedef  cs_mesh_msg_t TYPIFY(CMD_SEND_MESH_MSG);
typedef  internal_multi_switch_item_t TYPIFY(CMD_SEND_MESH_MSG_MULTI_SWITCH);
typedef  cs_mesh_model_msg_profile_location_t TYPIFY(CMD_SEND_MESH_MSG_PROFILE_LOCATION);
typedef  behaviour_settings_t TYPIFY(CMD_SEND_MESH_MSG_SET_BEHAVIOUR_SETTINGS);
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
typedef  void TYPIFY(EVT_SETUP_DONE);
typedef  session_nonce_t TYPIFY(EVT_SESSION_NONCE_SET);
typedef  state_external_stone_t TYPIFY(EVT_STATE_EXTERNAL_STONE);
typedef  void TYPIFY(EVT_STATE_FACTORY_RESET_DONE);
typedef  void TYPIFY(EVT_STORAGE_INITIALIZED);
typedef  cs_type_and_id_t TYPIFY(EVT_STORAGE_WRITE_DONE);
typedef  cs_type_and_id_t TYPIFY(EVT_STORAGE_REMOVE_DONE);
typedef  cs_state_id_t TYPIFY(EVT_STORAGE_REMOVE_ALL_TYPES_WITH_ID_DONE);
typedef  void TYPIFY(EVT_STORAGE_GC_DONE);
typedef  void TYPIFY(EVT_STORAGE_FACTORY_RESET_DONE);
typedef  void TYPIFY(EVT_STORAGE_PAGES_ERASED);
typedef  void TYPIFY(EVT_MESH_FACTORY_RESET_DONE);
typedef  void TYPIFY(EVT_SWITCH_FORCED_OFF);
typedef  bool TYPIFY(CMD_SWITCHING_ALLOWED);
typedef  uint32_t TYPIFY(EVT_TICK);
typedef  uint32_t TYPIFY(EVT_TIME_SET);
typedef  void TYPIFY(CMD_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN);
typedef SwitchBehaviour TYPIFY(CMD_ADD_BEHAVIOUR);
typedef std::tuple<uint8_t,SwitchBehaviour> TYPIFY(CMD_REPLACE_BEHAVIOUR);
typedef uint8_t TYPIFY(CMD_REMOVE_BEHAVIOUR); // index
typedef uint8_t TYPIFY(CMD_GET_BEHAVIOUR); // index
typedef void TYPIFY(CMD_GET_BEHAVIOUR_INDICES);
typedef void TYPIFY(CMD_GET_BEHAVIOUR_DEBUG);
typedef void TYPIFY(EVT_BEHAVIOURSTORE_MUTATION);
typedef BOOL TYPIFY(EVT_BEHAVIOUR_OVERRIDDEN);
typedef internal_register_tracked_device_packet_t TYPIFY(CMD_REGISTER_TRACKED_DEVICE);
typedef internal_update_tracked_device_packet_t TYPIFY(CMD_UPDATE_TRACKED_DEVICE);
typedef uint8_t /* PresenceHandler::MutationType */ TYPIFY(EVT_PRESENCE_MUTATION);
typedef bool TYPIFY(CMD_SET_RELAY);
typedef uint8_t TYPIFY(CMD_SET_DIMMER); // interpret as intensity value, not combined with relay state.
typedef void TYPIFY(EVT_GOING_TO_DFU);
typedef profile_location_t TYPIFY(EVT_PROFILE_LOCATION);
typedef cs_mesh_model_msg_sync_request_t TYPIFY(EVT_MESH_SYNC_REQUEST_OUTGOING);
typedef cs_mesh_model_msg_sync_request_t TYPIFY(EVT_MESH_SYNC_REQUEST_INCOMING);
typedef void TYPIFY(EVT_MESH_SYNC_FAILED);
typedef void TYPIFY(EVT_MESH_PAGES_ERASED);
typedef cs_mesh_model_msg_state_0_t TYPIFY(EVT_MESH_EXT_STATE_0);
typedef cs_mesh_model_msg_state_1_t TYPIFY(EVT_MESH_EXT_STATE_1);
typedef uint32_t TYPIFY(CMD_SEND_MESH_MSG_SET_TIME);
typedef void TYPIFY(CMD_SEND_MESH_MSG_NOOP);

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
 * Check if type can have multiple IDs.
 */
bool hasMultipleIds(CS_TYPE const & type);

/**
 * Check if type should be removed on factory reset.
 */
bool removeOnFactoryReset(CS_TYPE const & type, cs_state_id_t id);


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
