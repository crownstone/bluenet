/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 20 Apr., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

/* Current conventions:
 *
 * + American English (-ize, -or, -er, -ction)
 * + Start with object under consideration: "Buffer is locked", not "Locking of buffer"
 * + Use underscores to separate words
 */

/* BLE defines
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

#define BLE_CHAR_TEMPERATURE                     "Temperature"
#define BLE_CHAR_RESET                           "Reset"
#define BLE_CHAR_MESH                            "Mesh"
#define BLE_CHAR_CONFIG_SET                      "Config Set"
#define BLE_CHAR_CONFIG_SELECT                   "Config Select"
#define BLE_CHAR_CONFIG_GET                      "Config Get"
#define BLE_CHAR_RSSI                            "RSSI"
#define BLE_CHAR_SCAN                            "Scan"
#define BLE_CHAR_TRACK                           "Track"
#define BLE_CHAR_PWM                             "PWM"

/* Error messages
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
#define MSG_BLE_ADVERTISING_START                "Advertising start"
#define MSG_BLE_ADVERTISING_STARTED              "Advertising started"
#define MSG_BLE_NO_CUSTOM_SERVICES               "Services, no custom ones!"
#define MSG_BLE_ADVERTISEMENT_TOO_BIG            "Advertisement too big"
#define MSG_BLE_ADVERTISEMENT_CONFIG_INVALID     "Advertisement invalid config"
#define MSG_BLE_STACK_INITIALIZED                "Stack already initialized"

#define MSG_SERVICE_GENERAL_INIT                 "Service General init"
#define MSG_CHAR_TEMPERATURE_ADD                 "Characteristic Temperature added"
#define MSG_CHAR_TEMPERATURE_SKIP                "Characteristic Temperature skipped"
#define MSG_CHAR_RESET_ADD                       "Characteristic Reset added"
#define MSG_CHAR_RESET_SKIP                      "Characteristic Reset skipped"
#define MSG_CHAR_MESH_ADD                        "Characteristic Mesh added"
#define MSG_CHAR_MESH_SKIP                       "Characteristic Mesh skipped"
#define MSG_CHAR_CONFIGURATION_ADD               "Characteristic Configuration added"
#define MSG_CHAR_CONFIGURATION_SKIP              "Characteristic Configuration skipped"

