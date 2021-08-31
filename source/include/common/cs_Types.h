/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 23, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdint>
#include <type_traits>
#include <tuple>

#include "cfg/cs_Config.h"

#include "protocol/cs_CommandTypes.h"
#include "protocol/cs_ErrorCodes.h"
#include "structs/cs_PacketsInternal.h"
#include <structs/cs_BleCentralPackets.h>
#include <structs/cs_CrownstoneCentralPackets.h>

#include <behaviour/cs_SwitchBehaviour.h>
#include <behaviour/cs_TwilightBehaviour.h>
#include <behaviour/cs_ExtendedSwitchBehaviour.h>

#include <localisation/cs_Nearestnearestwitnessreport.h>
#include <localisation/cs_TrackableEvent.h>

#include <time/cs_TimeSyncMessage.h>
#include <mesh/cs_MeshMsgEvent.h>
#include <protocol/cs_AssetFilterPackets.h>

// #include <presence/cs_PresenceHandler.h>

enum TypeBases {
	State_Base   = 0x000,
	InternalBase = 0x100,

	// we keep internal events categorized so that the test suite
	// can easily send those while keeping flexibility in ordering this file.
	InternalBaseBluetooth = InternalBase + 0,
	InternalBaseSwitch = InternalBase + 20,
	InternalBasePower = InternalBase + 40,
	InternalBaseErrors = InternalBase + 60,
	InternalBaseStorage = InternalBase + 80,
	InternalBaseLogging = InternalBase + 100,
	InternalBaseADC = InternalBase + 120,
	InternalBaseMesh = InternalBase + 140,
	InternalBaseBehaviour = InternalBase + 170,
	InternalBaseLocalisation = InternalBase + 190,
	InternalBaseSystem = InternalBase + 210,
	InternalBaseTests = 0xF000,
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
	CONFIG_SCAN_INTERVAL_625US              = 39, // Scan interval in 625 µs units.
	CONFIG_SCAN_WINDOW_625US                = 40, // Scan window in 625 µs units.
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
	CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_DIMMER  = 51,
	CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_UP    = 52,
	CONFIG_PWM_TEMP_VOLTAGE_THRESHOLD_DOWN  = 53,
	CONFIG_DIMMING_ALLOWED                      = 54,
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
	STATE_IBEACON_CONFIG_ID                 = 154,
	STATE_MICROAPP                          = 155,
	STATE_SOFT_ON_SPEED                     = 156,
	STATE_HUB_MODE                          = 157,
	STATE_UART_KEY                          = 158,

	STATE_ASSET_FILTERS_VERSION             = 159,
	STATE_ASSET_FILTER_32                   = 160,
	STATE_ASSET_FILTER_64                   = 161,
	STATE_ASSET_FILTER_128                  = 162,
	STATE_ASSET_FILTER_256                  = 163,
	STATE_ASSET_FILTER_512                  = 164,

	/*
	 * Internal commands and events.
	 * Start at Internal_Base.
	 */

	// InternalBaseBluetooth
	EVT_ADV_BACKGROUND_PARSED = InternalBaseBluetooth,     // Sent when a background advertisement has been validated and parsed.
	EVT_DEVICE_SCANNED,                                    // Device was scanned.
	EVT_ADV_BACKGROUND,                                    // Background advertisement has been received.
	EVT_ADV_BACKGROUND_PARSED_V1,                          // Sent when a v1 background advertisement has been received.
	EVT_ADVERTISEMENT_UPDATED,                             // Advertisement was updated. TODO: advertisement data as payload?
	EVT_SCAN_STARTED,                                      // Scanner started scanning.
	EVT_SCAN_STOPPED,                                      // Scanner stopped scanning.
	EVT_BLE_CONNECT,                                       // Device connected.
	EVT_BLE_DISCONNECT,                                    // Device disconnected.
	CMD_ENABLE_ADVERTISEMENT,                              // Enable/disable advertising.

	// Switch (aggregator)
	CMD_SWITCH_OFF = InternalBaseSwitch,              // Turn switch off.
	CMD_SWITCH_ON,                                    // Turn switch on.
	CMD_SWITCH_TOGGLE,                                // Toggle switch.
	CMD_SWITCH,                                       // Set switch.
	CMD_SET_RELAY,                                    // Set the relay state.
	CMD_SET_DIMMER,                                   // Set the dimmer state.
	CMD_MULTI_SWITCH,                                 // Handle a multi switch.
	CMD_SWITCHING_ALLOWED,                            // Set switch lock.
	CMD_DIMMING_ALLOWED,                              // Set allow dimming.

	// Power
	EVT_DIMMER_POWERED = InternalBasePower,           // Dimmer being powered is changed. Payload: true when powered, and ready to be used.
	EVT_BROWNOUT_IMPENDING,                           // Brownout is impending (low chip supply voltage).

	// Errors
	EVT_CURRENT_USAGE_ABOVE_THRESHOLD = InternalBaseErrors,     // Current usage goes over the threshold.
	EVT_CURRENT_USAGE_ABOVE_THRESHOLD_DIMMER,                   // Current usage goes over the dimmer threshold, while dimmer is on.
	EVT_DIMMER_ON_FAILURE_DETECTED,                             // Dimmer leaks current, while it's supposed to be off.
	EVT_DIMMER_OFF_FAILURE_DETECTED,                            // Dimmer blocks current, while it's supposed to be on.
	EVT_CHIP_TEMP_ABOVE_THRESHOLD,                              // Chip temperature is above threshold.
	EVT_CHIP_TEMP_OK,                                           // Chip temperature is ok again.
	EVT_DIMMER_TEMP_ABOVE_THRESHOLD,                            // Dimmer temperature is above threshold.
	EVT_DIMMER_TEMP_OK,                                         // Dimmer temperature is ok again.
	EVT_DIMMER_FORCED_OFF,                                      // Dimmer was forced off.
	EVT_SWITCH_FORCED_OFF,                                      // Switch (relay and dimmer) was forced off.
	EVT_RELAY_FORCED_ON,                                        // Relay was forced on.

	// Storage
	EVT_STORAGE_INITIALIZED = InternalBaseStorage,    // Storage is initialized, storage is only usable after this event!
	EVT_STORAGE_WRITE_DONE,                           // An item has been written to storage.
	EVT_STORAGE_REMOVE_DONE,                          // An item has been invalidated at storage.
	EVT_STORAGE_REMOVE_ALL_TYPES_WITH_ID_DONE,        // All state values with a certain ID have been invalidated at storage.
	EVT_STORAGE_GC_DONE,                              // Garbage collection is done, invalidated data is actually removed at this point.
	EVT_STORAGE_FACTORY_RESET_DONE,                   // Factory reset of storage is done. /!\ Only to be used by State.
	EVT_STORAGE_PAGES_ERASED,                         // All storage pages are completely erased.
	CMD_FACTORY_RESET,                                // Perform a factory reset: clear all data.
	EVT_STATE_FACTORY_RESET_DONE,                     // Factory reset of state is done.
	EVT_MESH_FACTORY_RESET_DONE,                      // Factory reset of mesh storage is done.
	CMD_STORAGE_GARBAGE_COLLECT,                      // Start garbage collection of FDS.

	// Logging
	CMD_ENABLE_LOG_POWER = InternalBaseLogging,       // Enable/disable power calculations logging.
	CMD_ENABLE_LOG_CURRENT,                           // Enable/disable current samples logging.
	CMD_ENABLE_LOG_VOLTAGE,                           // Enable/disable voltage samples logging.
	CMD_ENABLE_LOG_FILTERED_CURRENT,                  // Enable/disable filtered current samples logging.

	// ADC config
	CMD_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN = InternalBaseADC,     // Toggle ADC voltage pin. TODO: pin as payload?
	CMD_ENABLE_ADC_DIFFERENTIAL_CURRENT,                            // Toggle differential mode on current pin.
	CMD_ENABLE_ADC_DIFFERENTIAL_VOLTAGE,                            // Toggle differential mode on voltage pin.
	CMD_INC_VOLTAGE_RANGE,                                          // Increase voltage range.
	CMD_DEC_VOLTAGE_RANGE,                                          // Decrease voltage range.
	CMD_INC_CURRENT_RANGE,                                          // Increase current range.
	CMD_DEC_CURRENT_RANGE,                                          // Decrease current range.
	EVT_ADC_RESTARTED,                                              // ADC has been restarted. Sent before the first buffer is to be processed.

	// Mesh
	CMD_SEND_MESH_MSG = InternalBaseMesh,             // Send a mesh message.
	CMD_SEND_MESH_MSG_SET_TIME,                       // Send a set time mesh message.
	CMD_SEND_MESH_MSG_NOOP,                           // Send a noop mesh message.
	CMD_SEND_MESH_MSG_MULTI_SWITCH,                   // Send a switch mesh message.
	CMD_SEND_MESH_MSG_PROFILE_LOCATION,               // Send a profile location mesh message.
	CMD_SEND_MESH_MSG_SET_BEHAVIOUR_SETTINGS,         // Send a set behaviour settings mesh message.
	CMD_SEND_MESH_MSG_TRACKED_DEVICE_REGISTER,
	CMD_SEND_MESH_MSG_TRACKED_DEVICE_TOKEN,
	CMD_SEND_MESH_MSG_TRACKED_DEVICE_LIST_SIZE,
	CMD_SEND_MESH_CONTROL_COMMAND,                    // Send a control command via the mesh. All permission checks must have been done already!
	CMD_ENABLE_MESH,                                  // Enable/disable mesh.
	EVT_MESH_TRACKED_DEVICE_REGISTER,                 // Mesh received a tracked device to register.
	EVT_MESH_TRACKED_DEVICE_TOKEN,                    // Mesh received a tracked device token.
	EVT_MESH_TRACKED_DEVICE_LIST_SIZE,                // Mesh received a tracked device list size.
	EVT_MESH_SYNC_REQUEST_OUTGOING,                   // Before an outgoing sync request is broadcasted, this event is fired internally so that other event handlers can tag on.
	EVT_MESH_SYNC_REQUEST_INCOMING,                   // When a sync request is received, this event is fired internally so that each event handler can individually respond to it.
	EVT_MESH_SYNC_FAILED,                             // When syncing is considered to have failed, no more retries.
	EVT_MESH_EXT_STATE_0,                             // Mesh received part 0 of the state of a Crownstone.
	EVT_MESH_EXT_STATE_1,                             // Mesh received part 1 of the state of a Crownstone.
	EVT_MESH_PAGES_ERASED,                            // All mesh storage pages are completely erased.
	CMD_SEND_MESH_MSG_TRACKED_DEVICE_HEARTBEAT,       // Send a tracked device heartbeat mesh message.
	EVT_MESH_TRACKED_DEVICE_HEARTBEAT,                // Mesh received a tracked device heartbeat.
	EVT_MESH_RSSI_PING,                               // TODO: remove this type, it's not used.
	EVT_MESH_RSSI_DATA,                               // TODO: remove this type, it's not used.
	EVT_MESH_TIME_SYNC,                               // A time sync message was received
	EVT_RECV_MESH_MSG,                                // A mesh message was received.
	EVT_MESH_NEAREST_WITNESS_REPORT,                  // Payload: MeshMsgEvent with type CS_MESH_MODEL_TYPE_REPORT_ASSET_[MAC,ID]. Originates from MeshMsgHandler.

	// Behaviour
	CMD_ADD_BEHAVIOUR = InternalBaseBehaviour,        // Add a behaviour.
	CMD_REPLACE_BEHAVIOUR,                            // Replace a behaviour.
	CMD_REMOVE_BEHAVIOUR,                             // Remove a behaviour.
	CMD_GET_BEHAVIOUR,                                // Get a behaviour.
	CMD_GET_BEHAVIOUR_INDICES,                        // Get a list of indices of active behaviours.
	CMD_GET_BEHAVIOUR_DEBUG,                          // Get info to debug behaviour. Multiple classes will handle this command to fill pieces of info.
	CMD_CLEAR_ALL_BEHAVIOUR,                          // Clear all behaviours in the store, including persisted flash entries.
	EVT_BEHAVIOURSTORE_MUTATION,                      // Sent by BehaviourStore, after a change to the stored behaviours.
	EVT_BEHAVIOUR_OVERRIDDEN,                         // Informs whether behaviour is overridden by user (in override state).

	// Localisation of devices
	CMD_REGISTER_TRACKED_DEVICE = InternalBaseLocalisation, // Register a tracked device.
	CMD_UPDATE_TRACKED_DEVICE,                        // Update data of a tracked device.
	EVT_RECEIVED_PROFILE_LOCATION,                    // Received the location of a profile via mesh or command, or emulated by cs_TrackedDevices.
	EVT_PRESENCE_MUTATION,                            // Presence changed.
	EVT_STATE_EXTERNAL_STONE,                         // The state of another stone has been received.
	CMD_TRACKED_DEVICE_HEARTBEAT,                     // Set location of a tracked device, with a TTL. This command can be sent instead of advertisements.
	EVT_PRESENCE_CHANGE,                              // The presence has changed. When the first user enters, multiple events will be sent.
	CMD_GET_PRESENCE,                                 // Get the current presence.

	CMD_UPLOAD_FILTER,                                // Update data chunk for a filter.                 See PROTOCOL.md CTRL_CMD_FILTER_UPLOAD
	CMD_REMOVE_FILTER,                                // Remove a filter by id.                          See PROTOCOL.md CTRL_CMD_FILTER_REMOVE
	CMD_COMMIT_FILTER_CHANGES,                        // Confirm all recent changes to filters.          See PROTOCOL.md CTRL_CMD_FILTER_COMMIT
	CMD_GET_FILTER_SUMMARIES,                         // Obtain status summary for each filter in RAM.   See PROTOCOL.md CTRL_CMD_FILTER_GET_SUMMARIES
	EVT_FILTERS_UPDATED,                              // Sent when the asset filter master version was updated (after a commit command was accepted).
	EVT_FILTER_MODIFICATION,                          // Sent when filter modification has started (payload is true) or stopped (payload is false).

	EVT_ASSET_ACCEPTED,                               // Sent by AssetFiltering when an incoming scan is accepted by a filter.


	// System
	CMD_RESET_DELAYED = InternalBaseSystem,           // Reboot scheduled with a (short) delay.
	EVT_GOING_TO_DFU,                                 // The system will reboot to DFU mode soon.

	CMD_SET_TIME,                                     // Set the time.
	CMD_SET_IBEACON_CONFIG_ID,                        // Set which ibeacon config id to use for advertising.
	EVT_TIME_SET,                                     // Time is set or changed. WARNING: this event is only sent on set time command. Payload: previous posix time
	EVT_TICK,                                         // Sent about every TICK_INTERVAL_MS ms.

	CMD_CONTROL_CMD,                                  // Handle a control command.
	EVT_SESSION_DATA_SET,                             // Session data and setup key are generated. Data pointer has to point to memory that stays valid!
	EVT_SETUP_DONE,                                   // Setup is done (and settings are stored).

	CMD_GET_ADC_RESTARTS,                             // Get number of ADC restarts.
	CMD_GET_SWITCH_HISTORY,                           // Get the switch command history.
	CMD_GET_POWER_SAMPLES,                            // Get power samples of interesting events.
	CMD_GET_SCHEDULER_MIN_FREE,                       // Get minimum queue space left of app scheduler observed so far.
	CMD_GET_RESET_REASON,                             // Get last reset reason. Contents of POWER->RESETREAS as it was on boot.
	CMD_GET_GPREGRET,                                 // Get the Nth general purpose retention register as it was on boot.
	CMD_GET_ADC_CHANNEL_SWAPS,                        // Get number of detected ADC channel swaps.
	CMD_GET_RAM_STATS,                                // Get RAM statistics.

	CMD_MICROAPP_GET_INFO,                            // Microapp control command.
	CMD_MICROAPP_UPLOAD,                              // Microapp control command. The data pointer is assume to remain valid until write is completed!
	CMD_MICROAPP_VALIDATE,                            // Microapp control command.
	CMD_MICROAPP_REMOVE,                              // Microapp control command.
	CMD_MICROAPP_ENABLE,                              // Microapp control command.
	CMD_MICROAPP_DISABLE,                             // Microapp control command.
	EVT_MICROAPP_UPLOAD_RESULT,                       // Uploaded chunk has been written to flash, or failed to do so.
	EVT_MICROAPP_ERASE_RESULT,                        // Microapp has been erase from flash, or failed to do so.
	CMD_MICROAPP_ADVERTISE,                           // A microapp wants to advertise something.

	CMD_BLE_CENTRAL_CONNECT,                          // Connect to a device.       See BleCentral::connect().
	CMD_BLE_CENTRAL_DISCONNECT,                       // Disconnect from device.    See BleCentral::disconnect().
	CMD_BLE_CENTRAL_DISCOVER,                         // Discover services.         See BleCentral::discoverServices().
	CMD_BLE_CENTRAL_READ,                             // Read a characteristic.     See BleCentral::read().
	CMD_BLE_CENTRAL_WRITE,                            // Write a characteristic.    See BleCentral::write().

	EVT_BLE_CENTRAL_CONNECT_START,                    // An outgoing connection is going to be made. Always followed by EVT_BLE_CENTRAL_CONNECT_RESULT.
	EVT_BLE_CENTRAL_CONNECT_RESULT,                   // Result of a connection attempt.
	EVT_BLE_CENTRAL_DISCONNECTED,                     // Outgoing connection was terminated. By request, or due to some error.
	EVT_BLE_CENTRAL_DISCOVERY,                        // A single service or characteristic is discovered.
	EVT_BLE_CENTRAL_DISCOVERY_RESULT,                 // Result of service discovery.
	EVT_BLE_CENTRAL_READ_RESULT,                      // Result of a read.
	EVT_BLE_CENTRAL_WRITE_RESULT,                     // Result of a write.
	EVT_BLE_CENTRAL_NOTIFICATION,                     // A notification has been received.

	CMD_CS_CENTRAL_CONNECT,                           // Connect to a device.       See CrownstoneCentral::connect().
	CMD_CS_CENTRAL_DISCONNECT,                        // Disconnect from device.    See CrownstoneCentral::disconnect().
	CMD_CS_CENTRAL_WRITE,                             // Write a control command.   See CrownstoneCentral::write().
	CMD_CS_CENTRAL_GET_WRITE_BUF,                     // Request the write buffer.  See CrownstoneCentral::requestWriteBuffer(). The result.buf will be set to the write buffer.

	EVT_CS_CENTRAL_CONNECT_RESULT,
	EVT_CS_CENTRAL_READ_RESULT,
	EVT_CS_CENTRAL_WRITE_RESULT,

	EVT_HUB_DATA_REPLY,                               // Sent when the hub data reply is received.
	
	CMD_MESH_TOPO_GET_MAC,                            // Get the MAC address of a given stone ID.
	EVT_MESH_TOPO_MAC_RESULT,                         // The resulting MAC address.
	CMD_MESH_TOPO_RESET,                              // Reset the stored mesh topology.
	CMD_MESH_TOPO_GET_RSSI,                           // Get the RSSI to a stoneId. The RSSI is set in the result.

	EVT_TWI_INIT,                                     // TWI initialisation.
	EVT_TWI_WRITE,                                    // TWI write.
	EVT_TWI_READ,                                     // TWI read (request).
	EVT_TWI_UPDATE,                                   // TWI update from TWI module to listeners (not implemented yet).

	EVT_GPIO_INIT,                                    // GPIO, init pin (eventually event handler)
	EVT_GPIO_WRITE,                                   // GPIO, write value
	EVT_GPIO_READ,                                    // GPIO, read value (directly)
	EVT_GPIO_UPDATE,                                  // GPIO, update other modules with read values

	CMD_TEST_SET_TIME = InternalBaseTests,            // Set time for testing.

	EVT_GENERIC_TEST = 0xFFFF,                        // Can be used by the python test python lib for ad hoc tests during development.
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

enum ResetCode {
	CS_RESET_CODE_SOFT_RESET = 0,
	CS_RESET_CODE_GO_TO_DFU_MODE = 1,
};

struct __attribute__((packed)) reset_delayed_t {
	uint8_t resetCode; // ResetCode
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

/**
 * Takes a pointer to a buffer and reinterprets it as pointer to the given type.
 */
#define CS_TYPE_CAST(EVT_NAME, PTR) reinterpret_cast<TYPIFY(EVT_NAME)*>(PTR)
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
typedef     BOOL TYPIFY(CONFIG_DIMMING_ALLOWED);
typedef uint16_t TYPIFY(CONFIG_RELAY_HIGH_DURATION);
typedef     BOOL TYPIFY(CONFIG_SCANNER_ENABLED);
typedef uint16_t TYPIFY(CONFIG_SCAN_BREAK_DURATION);
typedef uint16_t TYPIFY(CONFIG_SCAN_DURATION);
typedef uint16_t TYPIFY(CONFIG_SCAN_INTERVAL_625US);
typedef uint16_t TYPIFY(CONFIG_SCAN_WINDOW_625US);
typedef uint16_t TYPIFY(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD);
typedef uint16_t TYPIFY(CONFIG_SOFT_FUSE_CURRENT_THRESHOLD_DIMMER);
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
typedef sun_time_t TYPIFY(STATE_SUN_TIME);
typedef void TYPIFY(STATE_BEHAVIOUR_RULE);
typedef void TYPIFY(STATE_TWILIGHT_RULE);
typedef void TYPIFY(STATE_EXTENDED_BEHAVIOUR_RULE);
typedef behaviour_settings_t TYPIFY(STATE_BEHAVIOUR_SETTINGS);
typedef uint32_t TYPIFY(STATE_BEHAVIOUR_MASTER_HASH);
typedef cs_mesh_iv_index_t TYPIFY(STATE_MESH_IV_INDEX);
typedef cs_mesh_seq_number_t TYPIFY(STATE_MESH_SEQ_NUMBER);
typedef ibeacon_config_id_packet_t TYPIFY(STATE_IBEACON_CONFIG_ID);
typedef microapp_state_t TYPIFY(STATE_MICROAPP);
typedef uint8_t TYPIFY(STATE_SOFT_ON_SPEED);
typedef uint8_t TYPIFY(STATE_HUB_MODE);
typedef asset_filters_version_t TYPIFY(STATE_ASSET_FILTERS_VERSION);

typedef  void TYPIFY(EVT_ADC_RESTARTED);
typedef  adv_background_t TYPIFY(EVT_ADV_BACKGROUND);
typedef  adv_background_parsed_t TYPIFY(EVT_ADV_BACKGROUND_PARSED);
typedef  adv_background_parsed_v1_t TYPIFY(EVT_ADV_BACKGROUND_PARSED_V1);
typedef  void TYPIFY(EVT_ADVERTISEMENT_UPDATED);
typedef  ble_central_connect_t TYPIFY(CMD_BLE_CENTRAL_CONNECT);
typedef  void TYPIFY(CMD_BLE_CENTRAL_DISCONNECT);
typedef  ble_central_discover_t TYPIFY(CMD_BLE_CENTRAL_DISCOVER);
typedef  ble_central_read_t TYPIFY(CMD_BLE_CENTRAL_READ);
typedef  ble_central_write_t TYPIFY(CMD_BLE_CENTRAL_WRITE);
typedef  void TYPIFY(EVT_BLE_CONNECT);
typedef  void TYPIFY(EVT_BLE_DISCONNECT);
typedef  void TYPIFY(EVT_BLE_CENTRAL_CONNECT_START);
typedef  cs_ret_code_t TYPIFY(EVT_BLE_CENTRAL_CONNECT_RESULT);
typedef  void TYPIFY(EVT_BLE_CENTRAL_DISCONNECTED);
typedef  ble_central_discovery_t TYPIFY(EVT_BLE_CENTRAL_DISCOVERY);
typedef  cs_ret_code_t TYPIFY(EVT_BLE_CENTRAL_DISCOVERY_RESULT);
typedef  ble_central_read_result_t TYPIFY(EVT_BLE_CENTRAL_READ_RESULT);
typedef  cs_ret_code_t TYPIFY(EVT_BLE_CENTRAL_WRITE_RESULT);
typedef  ble_central_notification_t TYPIFY(EVT_BLE_CENTRAL_NOTIFICATION);
typedef  cs_central_connect_t TYPIFY(CMD_CS_CENTRAL_CONNECT);
typedef  void TYPIFY(CMD_CS_CENTRAL_DISCONNECT);
typedef  cs_central_write_t TYPIFY(CMD_CS_CENTRAL_WRITE);
typedef  void TYPIFY(CMD_CS_CENTRAL_GET_WRITE_BUF);
typedef  cs_ret_code_t TYPIFY(EVT_CS_CENTRAL_CONNECT_RESULT);
typedef  cs_central_write_result_t TYPIFY(EVT_CS_CENTRAL_READ_RESULT);
typedef  cs_central_write_result_t TYPIFY(EVT_CS_CENTRAL_WRITE_RESULT);
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
typedef  cs_mesh_model_msg_device_register_t TYPIFY(EVT_MESH_TRACKED_DEVICE_REGISTER);
typedef  cs_mesh_model_msg_device_register_t TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_REGISTER);
typedef  cs_mesh_model_msg_device_token_t TYPIFY(EVT_MESH_TRACKED_DEVICE_TOKEN);
typedef  cs_mesh_model_msg_device_token_t TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_TOKEN);
typedef  cs_mesh_model_msg_device_heartbeat_t TYPIFY(EVT_MESH_TRACKED_DEVICE_HEARTBEAT);
typedef  cs_mesh_model_msg_device_heartbeat_t TYPIFY(CMD_SEND_MESH_MSG_TRACKED_DEVICE_HEARTBEAT);
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
typedef  session_data_t TYPIFY(EVT_SESSION_DATA_SET);
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
typedef  void TYPIFY(CMD_STORAGE_GARBAGE_COLLECT);
typedef  void TYPIFY(EVT_SWITCH_FORCED_OFF);
typedef  bool TYPIFY(CMD_SWITCHING_ALLOWED);
typedef  uint32_t TYPIFY(EVT_TICK);
typedef  uint32_t TYPIFY(EVT_TIME_SET);
typedef  void TYPIFY(CMD_TOGGLE_ADC_VOLTAGE_VDD_REFERENCE_PIN);

// Behaviour
typedef SwitchBehaviour TYPIFY(CMD_ADD_BEHAVIOUR);
typedef std::tuple<uint8_t,SwitchBehaviour> TYPIFY(CMD_REPLACE_BEHAVIOUR);
typedef uint8_t TYPIFY(CMD_REMOVE_BEHAVIOUR); // index
typedef uint8_t TYPIFY(CMD_GET_BEHAVIOUR); // index
typedef void TYPIFY(CMD_GET_BEHAVIOUR_INDICES);
typedef void TYPIFY(CMD_GET_BEHAVIOUR_DEBUG);
typedef void TYPIFY(EVT_BEHAVIOURSTORE_MUTATION);
typedef BOOL TYPIFY(EVT_BEHAVIOUR_OVERRIDDEN);

// Localisation
typedef internal_register_tracked_device_packet_t TYPIFY(CMD_REGISTER_TRACKED_DEVICE);
typedef internal_update_tracked_device_packet_t TYPIFY(CMD_UPDATE_TRACKED_DEVICE);
typedef internal_tracked_device_heartbeat_packet_t TYPIFY(CMD_TRACKED_DEVICE_HEARTBEAT);
typedef uint8_t /* PresenceHandler::MutationType */ TYPIFY(EVT_PRESENCE_MUTATION);
typedef presence_change_t TYPIFY(EVT_PRESENCE_CHANGE);
typedef void TYPIFY(CMD_GET_PRESENCE);
typedef profile_location_t TYPIFY(EVT_RECEIVED_PROFILE_LOCATION);

typedef asset_filter_cmd_upload_filter_t TYPIFY(CMD_UPLOAD_FILTER);
typedef asset_filter_cmd_remove_filter_t TYPIFY(CMD_REMOVE_FILTER);
typedef asset_filter_cmd_commit_filter_changes_t TYPIFY(CMD_COMMIT_FILTER_CHANGES);
typedef void TYPIFY(CMD_GET_FILTER_SUMMARIES);
typedef void TYPIFY(EVT_FILTERS_UPDATED);
typedef bool TYPIFY(EVT_FILTER_MODIFICATION);
typedef AssetAcceptedEvent TYPIFY(EVT_ASSET_ACCEPTED);

typedef bool TYPIFY(CMD_SET_RELAY);
typedef uint8_t TYPIFY(CMD_SET_DIMMER); // interpret as intensity value, not combined with relay state.
typedef void TYPIFY(EVT_GOING_TO_DFU);

// Mesh
typedef cs_mesh_model_msg_sync_request_t TYPIFY(EVT_MESH_SYNC_REQUEST_OUTGOING);
typedef cs_mesh_model_msg_sync_request_t TYPIFY(EVT_MESH_SYNC_REQUEST_INCOMING);
typedef void TYPIFY(EVT_MESH_SYNC_FAILED);
typedef void TYPIFY(EVT_MESH_PAGES_ERASED);
typedef mesh_state_part_0_t TYPIFY(EVT_MESH_EXT_STATE_0);
typedef mesh_state_part_1_t TYPIFY(EVT_MESH_EXT_STATE_1);
typedef uint32_t TYPIFY(CMD_SEND_MESH_MSG_SET_TIME);
typedef set_ibeacon_config_id_packet_t TYPIFY(CMD_SET_IBEACON_CONFIG_ID);
typedef void TYPIFY(CMD_SEND_MESH_MSG_NOOP);
typedef void TYPIFY(CMD_GET_ADC_RESTARTS);
typedef void TYPIFY(CMD_GET_SWITCH_HISTORY);
typedef cs_power_samples_request_t TYPIFY(CMD_GET_POWER_SAMPLES);
typedef void TYPIFY(CMD_GET_SCHEDULER_MIN_FREE);
typedef void TYPIFY(CMD_GET_RESET_REASON);
typedef uint8_t TYPIFY(CMD_GET_GPREGRET);
typedef void TYPIFY(CMD_GET_ADC_CHANNEL_SWAPS);
typedef void TYPIFY(CMD_GET_RAM_STATS);
typedef void TYPIFY(CMD_MICROAPP_GET_INFO);
typedef microapp_upload_internal_t TYPIFY(CMD_MICROAPP_UPLOAD);
typedef microapp_ctrl_header_t TYPIFY(CMD_MICROAPP_VALIDATE);
typedef microapp_ctrl_header_t TYPIFY(CMD_MICROAPP_REMOVE);
typedef microapp_ctrl_header_t TYPIFY(CMD_MICROAPP_ENABLE);
typedef microapp_ctrl_header_t TYPIFY(CMD_MICROAPP_DISABLE);
typedef cs_ret_code_t TYPIFY(EVT_MICROAPP_UPLOAD_RESULT);
typedef cs_ret_code_t TYPIFY(EVT_MICROAPP_ERASE_RESULT);
typedef microapp_advertise_request_t TYPIFY(CMD_MICROAPP_ADVERTISE);
typedef uint32_t TYPIFY(CMD_TEST_SET_TIME);
typedef MeshMsgEvent TYPIFY(EVT_MESH_RSSI_PING);
typedef MeshMsgEvent TYPIFY(EVT_MESH_RSSI_DATA);
typedef time_sync_message_t TYPIFY(EVT_MESH_TIME_SYNC);
typedef MeshMsgEvent TYPIFY(EVT_RECV_MESH_MSG);
typedef hub_data_reply_t TYPIFY(EVT_HUB_DATA_REPLY);
typedef stone_id_t TYPIFY(CMD_MESH_TOPO_GET_MAC);
typedef mesh_topo_mac_result_t TYPIFY(EVT_MESH_TOPO_MAC_RESULT);
typedef void TYPIFY(CMD_MESH_TOPO_RESET);
typedef stone_id_t TYPIFY(CMD_MESH_TOPO_GET_RSSI);
typedef MeshMsgEvent TYPIFY(EVT_MESH_NEAREST_WITNESS_REPORT);

// TWI / I2C module
typedef cs_twi_init_t TYPIFY(EVT_TWI_INIT);
typedef cs_twi_write_t TYPIFY(EVT_TWI_WRITE);
typedef cs_twi_read_t TYPIFY(EVT_TWI_READ);
typedef uint8_t TYPIFY(EVT_TWI_UPDATE);

typedef cs_gpio_init_t TYPIFY(EVT_GPIO_INIT);
typedef cs_gpio_write_t TYPIFY(EVT_GPIO_WRITE);
typedef cs_gpio_read_t TYPIFY(EVT_GPIO_READ);
typedef cs_gpio_update_t TYPIFY(EVT_GPIO_UPDATE);

/**
 * The size of a particular default value. In case of strings or arrays this is the maximum size of the corresponding
 * field. There are no fields that are of unrestricted size. For fields that are not implemented it is possible to
 * set size to 0.
 */
size16_t TypeSize(CS_TYPE const & type);

/**
 * Check if type can have multiple IDs.
 */
bool hasMultipleIds(CS_TYPE const & type);

/**
 * Check if type should be removed on factory reset.
 */
bool removeOnFactoryReset(CS_TYPE const & type, cs_state_id_t id);

/**
 * Gives the required access level to set a state type.
 */
EncryptionAccessLevel getUserAccessLevelSet(CS_TYPE const & type);

/**
 * Gives the required access level to get a state type.
 */
EncryptionAccessLevel getUserAccessLevelGet(CS_TYPE const & type);
