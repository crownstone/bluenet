/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 20 Apr., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
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
#define BLE_SERVICE_GENERAL                      "General Service"
#define BLE_SERVICE_INDOOR_LOCALIZATION          "Indoor Localization Service"
#define BLE_SERVICE_POWER                        "Power Service"
#define BLE_SERVICE_GENERIC                      "Generic Service"
#define BLE_SERVICE_BATTERY                      "Battery Service"
#define BLE_SERVICE_ALERT                        "Alert Service"
#define BLE_SERVICE_SCHEDULE                     "Schedule Service"
#define BLE_SERVICE_DEVICE_INFORMATION           "Device Information"

#define BLE_CHAR_TEMPERATURE                     "Temperature"
#define BLE_CHAR_RESET                           "Reset"
#define BLE_CHAR_MESH                            "Mesh"
#define BLE_CHAR_CONFIG_WRITE                    "Config Write"
#define BLE_CHAR_CONFIG_READ                     "Config Read"
#define BLE_CHAR_RSSI                            "RSSI"
#define BLE_CHAR_SCAN                            "Scan"
#define BLE_CHAR_TRACK                           "Track"
#define BLE_CHAR_PWM                             "PWM"
#define BLE_CHAR_RELAY                           "Relay"
#define BLE_CHAR_NEW_ALERT                       "New Alert"
#define BLE_CHAR_CURRENT_TIME                    "Current Time"
#define BLE_CHAR_WRITE_SCHEDULE                  "Write schedule"
#define BLE_CHAR_LIST_SCHEDULE                   "List schedule"
#define BLE_CHAR_STATEVAR_WRITE                  "StateVar Write"
#define BLE_CHAR_STATEVAR_READ                   "StateVar Read"

/** Error messages
 *
 * Keep them concise. These shouldn't be part of the heap, only of the stack, and they contribute to code size.
 */
#define MSG_NAME_TOO_LONG                        "Name is too long."
#define MSG_STACK_UNDEFINED                      "Stack is undefined."
#define MSG_BUFFER_IS_LOCKED                     "Buffer is locked."
#define MSG_CHAR_VALUE_UNDEFINED                 "Characteristic value is undefined"
#define MSG_CHAR_VALUE_WRITE                     "Characteristic value is being written"
#define MSG_CHAR_VALUE_WRITTEN                   "Characteristic value is written"
#define MSG_MESH_MESSAGE_WRITE                   "Mesh message being written"
#define MSG_FIRMWARE_UPDATE                      "Firmware, perform update"
#define MSG_RESET                                "Reset"

#define MSG_BLE_CHAR_TOO_MANY                    "Error characteristic: Too many added to single service"
#define MSG_BLE_CHAR_CREATION_ERROR              "Error characteristic: Creation"
#define MSG_BLE_CHAR_CANNOT_FIND                 "Error characteristic: Cannot find"
#define MSG_BLE_CHAR_INITIALIZED                 "Error characteristic: Already initialized"
#define MSG_BLE_SOFTDEVICE_RUNNING               "Softdevice is already running"
#define MSG_BLE_SOFTDEVICE_INIT                  "Softdevice init"
#define MSG_BLE_SOFTDEVICE_ENABLE                "Softdevice enable"
#define MSG_BLE_SOFTDEVICE_ENABLE_GAP            "Softdevice enable GAP"
#define MSG_BLE_CONFIGURE_BLEDEVICE              "Configuring as BleDevice"
#define MSG_BLE_CONFIGURE_IBEACON                "Configuring as iBeacon"
#define MSG_BLE_ADVERTISING_STARTING             "Start advertising"
#define MSG_BLE_ADVERTISING_STARTED              "Advertising started"
#define MSG_BLE_IBEACON_STARTED                  "iBeacon advertising started"
#define MSG_BLE_NO_CUSTOM_SERVICES               "Services, no custom ones!"
#define MSG_BLE_ADVERTISEMENT_TOO_BIG            "Advertisement too big"
#define MSG_BLE_ADVERTISEMENT_CONFIG_INVALID     "Advertisement invalid config"
#define MSG_BLE_STACK_INITIALIZED                "Stack already initialized"

#define MSG_SERVICE_GENERAL_INIT                 "Service General init"
#define MSG_CHAR_CONTROL_ADD                     "Characteristic Control added"
#define MSG_CHAR_CONTROL_SKIP                    "Characteristic Control skipped"
#define MSG_CHAR_TEMPERATURE_ADD                 "Characteristic Temperature added"
#define MSG_CHAR_TEMPERATURE_SKIP                "Characteristic Temperature skipped"
#define MSG_CHAR_RESET_ADD                       "Characteristic Reset added"
#define MSG_CHAR_RESET_SKIP                      "Characteristic Reset skipped"
#define MSG_CHAR_MESH_ADD                        "Characteristic Mesh added"
#define MSG_CHAR_MESH_SKIP                       "Characteristic Mesh skipped"
#define MSG_CHAR_CONFIGURATION_ADD               "Characteristic Configuration added"
#define MSG_CHAR_CONFIGURATION_SKIP              "Characteristic Configuration skipped"
#define MSG_CHAR_STATEVARIABLES_ADD              "Characteristic StateVariables added"
#define MSG_CHAR_STATEVARIABLES_SKIP             "Characteristic StateVariables skipped"

