/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 20 Apr., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

/** Current conventions:
 *
 * + American English (-ize, -or, -er, -ction)
 * + Start with object under consideration: "Buffer is locked", not "Locking of buffer"
 * + Use underscores to separate words
 */

/** BLE defines
 *
 * Strings for services and characteristics are read by a developer if he/she interacts on a low-level with a BLE
 * device via e.g. the Nordic BLE app. These strings are stored currently in memory allocated by the SoftDevice.
 * Keep it as short as possible to maximize the number of services / characteristics available.
 */
#define BLE_SERVICE_GENERAL "General"
#define BLE_SERVICE_INDOOR_LOCALIZATION "Indoor Localization"
#define BLE_SERVICE_POWER "Power"
#define BLE_SERVICE_GENERIC "Generic"
#define BLE_SERVICE_BATTERY "Battery"
#define BLE_SERVICE_ALERT "Alert"
#define BLE_SERVICE_SCHEDULE "Schedule"
#define BLE_SERVICE_DEVICE_INFORMATION "Device Information"
#define BLE_SERVICE_CROWNSTONE "Crownstone"
#define BLE_SERVICE_SETUP "Setup"

#define BLE_CHAR_TEMPERATURE "Temperature"
#define BLE_CHAR_RESET "Reset"
#define BLE_CHAR_MESH_CONTROL "Mesh Control"
#define BLE_CHAR_CONTROL "Control"
#define BLE_CHAR_CONFIG_CONTROL "Config Control"
#define BLE_CHAR_CONFIG_READ "Config Read"
#define BLE_CHAR_STATE_CONTROL "State Control"
#define BLE_CHAR_STATE_READ "State Read"
#define BLE_CHAR_SESSION_DATA "Session data"
#define BLE_CHAR_FACTORY_RESET "Recovery"
#define BLE_CHAR_RSSI "RSSI"
#define BLE_CHAR_SCAN_CONTROL "Scan Control"
#define BLE_CHAR_SCAN_LIST "Scanned Devices"
#define BLE_CHAR_TRACK "Track Control"
#define BLE_CHAR_TRACK_LIST "Tracked Devices"
#define BLE_CHAR_PWM "PWM"
#define BLE_CHAR_RELAY "Relay"
#define BLE_CHAR_POWER_SAMPLES "Power Samples"
#define BLE_CHAR_POWER_CONSUMPTION "Power Consumption"
#define BLE_CHAR_NEW_ALERT "New Alert"
#define BLE_CHAR_CURRENT_TIME "Current Time"
#define BLE_CHAR_WRITE_SCHEDULE "Write schedule"
#define BLE_CHAR_LIST_SCHEDULE "List schedule"
#define BLE_CHAR_HARDWARE_REVISION "Hardware Revision"
#define BLE_CHAR_FIRMWARE_REVISION "Firmware Revision"
#define BLE_CHAR_SOFTWARE_REVISION "Software Revision"
#define BLE_CHAR_MAC_ADDRES "MAC Address"
#define BLE_CHAR_SETUP_KEY "Setup Encryption Key"
#define BLE_CHAR_GOTO_DFU "GoTo DFU"

#define STR_HANDLE_COMMAND "Handle command "
#define STR_ENABLE "Enable"
#define STR_DISABLE "Disable"
#define STR_FAILED "Failed"

// TODO: A lot of repetition, what is the difference between BLE_CHAR_HARDWARE_REVISION and STR_CHAR_HARDWARE_REVISION
//#define STR_CROWNSTONE                           "Crownstone"
#define STR_CHAR_CONTROL "Control"
#define STR_CHAR_RESULT "Result"
#define STR_CHAR_MESH "Mesh"
#define STR_CHAR_CONFIGURATION "Configuration"
#define STR_CHAR_STATE "State"
#define STR_CHAR_SESSION_DATA "Session data"
#define STR_CHAR_FACTORY_RESET "Recovery"

#define STR_CHAR_NEW_ALERT "New Alert"
#define STR_CHAR_HARDWARE_REVISION "Hardware Revision"
#define STR_CHAR_FIRMWARE_REVISION "Firmware Revision"
#define STR_CHAR_SOFTWARE_REVISION "Software Revision"
#define STR_CHAR_STATE_CONTROL "State Control"
#define STR_CHAR_TEMPERATURE "Temperature"
#define STR_CHAR_RESET "Reset"
#define STR_CHAR_RSSI "Signal Strength"
#define STR_CHAR_SCAN_DEVICES "Scan Devices"
#define STR_CHAR_TRACKED_DEVICE "Tracked Devices"
#define STR_CHAR_PWM "PWM"
#define STR_CHAR_RELAY "Relay"
#define STR_CHAR_POWER_SAMPLE "Power Sample"
#define STR_CHAR_CURRENT_CONSUMPTION "Current Consumption"
#define STR_CHAR_CURRENT_TIME "Current Time"
#define STR_CHAR_SCHEDULE "Schedule"
#define STR_CHAR_MAC_ADDRESS "MAC Address"
#define STR_CHAR_SETUP_KEY "Setup Encryption Key"
#define STR_CHAR_GOTO_DFU "GoTo DFU"

#define STR_CREATE_ALL_SERVICES "Create all services"

#define STR_ERR_BUFFER_NOT_LARGE_ENOUGH "Buffer not large enough!"
#define STR_ERR_ALLOCATE_MEMORY "Could not allocate memory!"
#define STR_ERR_VALUE_TOO_LONG "Value too long!"
#define STR_ERR_FORGOT_TO_ASSIGN_STACK "Forgot to assign stack!"
#define STR_ERR_ALREADY_STOPPED "Already stopped!"
#define STR_ERR_MULTIPLE_OF_16 "Data must be multiple of 16B"
#define STR_ERR_NOT_INITIALIZED "Not initialized"

/** Error messages
 *
 * Keep them concise. These shouldn't be part of the heap, only of the stack, and they contribute to code size.
 */
#define MSG_NAME_TOO_LONG "Name is too long."
#define MSG_STACK_UNDEFINED "Stack is undefined."
#define MSG_BUFFER_IS_LOCKED "Buffer is locked."
#define MSG_CHAR_VALUE_UNDEFINED "Characteristic value is undefined"
#define MSG_CHAR_VALUE_WRITE "Characteristic value is being written"
#define MSG_CHAR_VALUE_WRITTEN "Characteristic value is written"
#define MSG_MESH_MESSAGE_WRITE "Mesh message being written"
#define MSG_FIRMWARE_UPDATE "Firmware, perform update"
#define MSG_RESET "Reset"

#define MSG_BLE_CHAR_TOO_MANY "Error characteristic: Too many added to single service"
#define MSG_BLE_CHAR_CREATION_ERROR "Error characteristic: Creation"
#define MSG_BLE_CHAR_CANNOT_FIND "Error characteristic: Cannot find"
#define MSG_BLE_CHAR_INITIALIZED "Error characteristic: Already initialized"
#define MSG_BLE_SOFTDEVICE_RUNNING "Softdevice is already running"
#define MSG_BLE_SOFTDEVICE_INIT "Softdevice init"
#define MSG_BLE_SOFTDEVICE_ENABLE "Softdevice enable"
#define MSG_BLE_SOFTDEVICE_ENABLE_GAP "Softdevice enable GAP"
#define MSG_BLE_ADVERTISING_STARTING "Start advertising"
#define MSG_BLE_ADVERTISING_STARTED "Advertising started"
#define MSG_BLE_ADVERTISING_STOPPED "Advertising stopped"
#define MSG_BLE_NO_CUSTOM_SERVICES "Services, no custom ones!"
#define MSG_BLE_ADVERTISEMENT_TOO_BIG "Advertisement too big"
#define MSG_BLE_ADVERTISEMENT_CONFIG_INVALID "Advertisement invalid config"
#define MSG_BLE_STACK_INITIALIZED "Stack already initialized"

#define FMT_SERVICE_INIT "Init service "
#define FMT_CHAR_ADD "Added characteristic "
//#define FMT_CHAR_SKIP                            "Characteristic %s skipped"
//#define FMT_CHAR_REMOVED                         "Characteristic %s removed"
#define FMT_CHAR_EXISTS "Characteristic exists "
//#define FMT_CHAR_DOES_NOT_EXIST                  "Characteristic %s does not exist"

#define FMT_WRONG_PAYLOAD_LENGTH "Wrong payload length received: %u (should be %u)"
#define FMT_ZERO_PAYLOAD_LENGTH "Wrong payload length received: %u (should not be zero)"
//#define FMT_SELECT_TYPE                          "Select %s type: %d"
//#define FMT_WRITE_TYPE                           "Write %s type: %d"

#define FMT_HEADER "---- "
#define FMT_CREATE "Create "
#define FMT_INIT "Init "
#define FMT_ENABLE "Enable "
#define FMT_START "Start "
#define FMT_STOP "Stop "
#define FMT_NOT_INITIALIZED "Not initialized "
//#define FMT_ALREADY_INITIALIZED                  "%s already initialized!"
//#define FMT_ERR_EXPECTED                         "Expected %s"
//#define FMT_ERR_EXPECTED_RECEIVED                "Expected length %d, received: %d"
//#define FMT_SET_INT_TYPE_VAL                     "Set type=%d to %d"
//#define FMT_SET_INT_VAL                          "Set %s to %d"
//#define FMT_SET_FLOAT_TYPE_VAL                   "Set type=%d to %f"
//#define FMT_SET_FLOAT_VAL                        "Set %s to %f"
//#define FMT_SET_STR_TYPE_VAL                     "Set type=%d to %s"
//#define FMT_SET_STR_VAL                          "Set %s to %s"
//#define FMT_GET_INT_VAL                          "Get %s: %d"
//#define FMT_GET_STR_VAL                          "Get %s: %s"

#define FMT_CONFIGURATION_NOT_FOUND "There is no such configuration type (%d)."
#define FMT_STATE_NOT_FOUND "There is no such state variable (%d)."
#define FMT_VERIFICATION_FAILED "Verification failed (%d)"

#define FMT_USE_DEFAULT_VALUE "Use default value"
#define FMT_FOUND_STORED_VALUE "Found stored value: %d"
#define FMT_RAW_VALUE "Raw value: %02X %02X %02X %02X"

//#define FMT_BUFFER_NOT_ASSIGNED                  "%s buffer not assigned!"
//#define FMT_BUFFER_FULL                          "%s buffer full!"

//#define FMT_FAILED_TO                            "Failed to %s (%d)"
#define FMT_ALREADY "Already "

//#define FMT_BLE_CONFIGURE_AS                     "Configuring as %s"

#define FMT_ALLOCATE_MEMORY "Allocated memory at %p"
#define FMT_ERR_ASSIGN_BUFFER "Could not assign buffer at %p with size %d"
#define FMT_ASSIGN_BUFFER_LEN "Assign buffer at %p, len: %d"
