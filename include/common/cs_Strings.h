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

#define BLE_CHAR_TEMPERATURE                     "Temperature"
#define BLE_CHAR_RESET                           "Reset"
#define BLE_CHAR_MESH                            "Mesh"
#define BLE_CHAR_CONFIG_SET                      "Config Set"
#define BLE_CHAR_CONFIG_SELECT                   "Config Select"
#define BLE_CHAR_CONFIG_GET                      "Config Get"

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
