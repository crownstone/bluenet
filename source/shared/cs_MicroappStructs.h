/*
 * Microapp structs.
 *
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 10, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

const uint8_t MAX_PAYLOAD = 32;

enum CommandMicroappPin {
	CS_MICROAPP_COMMAND_PIN_SWITCH        = 0x00,
	CS_MICROAPP_COMMAND_PIN_DIMMER        = 0x00, // same one
	CS_MICROAPP_COMMAND_PIN_GPIO1         = 0x01, // GPIO pin on connector
	CS_MICROAPP_COMMAND_PIN_GPIO2         = 0x02, // GPIO pin on connector
	CS_MICROAPP_COMMAND_PIN_GPIO3         = 0x03, // GPIO pin on connector
	CS_MICROAPP_COMMAND_PIN_GPIO4         = 0x04, // GPIO pin on connector
	CS_MICROAPP_COMMAND_PIN_BUTTON1       = 0x05, // BUTTON1
	CS_MICROAPP_COMMAND_PIN_BUTTON2       = 0x06, // BUTTON2
	CS_MICROAPP_COMMAND_PIN_BUTTON3       = 0x07, // BUTTON3
	CS_MICROAPP_COMMAND_PIN_BUTTON4       = 0x08, // BUTTON4
	CS_MICROAPP_COMMAND_PIN_LED1          = 0x09, // LED1
	CS_MICROAPP_COMMAND_PIN_LED2          = 0x0a, // LED2
	CS_MICROAPP_COMMAND_PIN_LED3          = 0x0b, // LED3
	CS_MICROAPP_COMMAND_PIN_LED4          = 0x0c, // LED4
};

enum ErrorCodesMicroapp {
	ERR_NO_PAYLOAD                        = 0x01,    // need at least an opcode in the payload
	ERR_TOO_LARGE                         = 0x02,
};

enum CommandMicroapp {
	CS_MICROAPP_COMMAND_LOG               = 0x01,
	CS_MICROAPP_COMMAND_DELAY             = 0x02,
	CS_MICROAPP_COMMAND_PIN               = 0x03,
	CS_MICROAPP_COMMAND_SERVICE_DATA      = 0x04,
	CS_MICROAPP_COMMAND_TWI               = 0x05,
};

enum CommandMicroappLogOption {
	CS_MICROAPP_COMMAND_LOG_NEWLINE       = 0x00,
	CS_MICROAPP_COMMAND_LOG_NO_NEWLINE    = 0x01,
};

enum CommandMicroappLog {
	CS_MICROAPP_COMMAND_LOG_CHAR          = 0x00,
	CS_MICROAPP_COMMAND_LOG_INT           = 0x01,
	CS_MICROAPP_COMMAND_LOG_STR           = 0x02,
	CS_MICROAPP_COMMAND_LOG_ARR           = 0x03,
	CS_MICROAPP_COMMAND_LOG_FLOAT         = 0x04,
	CS_MICROAPP_COMMAND_LOG_DOUBLE        = 0x05,
	CS_MICROAPP_COMMAND_LOG_UINT          = 0x06,
	CS_MICROAPP_COMMAND_LOG_SHORT         = 0x07,
};

enum CommandMicroappPinOpcode1 {
	CS_MICROAPP_COMMAND_PIN_MODE          = 0x00,
	CS_MICROAPP_COMMAND_PIN_ACTION        = 0x01,
};

enum CommandMicroappPinOpcode2 {
	CS_MICROAPP_COMMAND_PIN_READ          = 0x01,
	CS_MICROAPP_COMMAND_PIN_WRITE         = 0x02,
	CS_MICROAPP_COMMAND_PIN_TOGGLE        = 0x03,
	CS_MICROAPP_COMMAND_PIN_INPUT_PULLUP  = 0x04,
};

enum CommandMicroappTwiOpcode {
	CS_MICROAPP_COMMAND_TWI_READ          = 0x01,
	CS_MICROAPP_COMMAND_TWI_WRITE         = 0x02,
	CS_MICROAPP_COMMAND_TWI_INIT          = 0x03,
	CS_MICROAPP_COMMAND_TWI_ENABLE        = 0x04,
	CS_MICROAPP_COMMAND_TWI_DISABLE       = 0x05,
};

enum CommandMicroappPinValue {
	CS_MICROAPP_COMMAND_VALUE_OFF         = 0x00,
	CS_MICROAPP_COMMAND_VALUE_ON          = 0x01,
	CS_MICROAPP_COMMAND_VALUE_CHANGE      = 0x02,
	CS_MICROAPP_COMMAND_VALUE_RISING      = 0x03,
	CS_MICROAPP_COMMAND_VALUE_FALLING     = 0x04,
};

/*
 * The structure used for communication between microapp and bluenet.
 */
typedef struct {
	uint8_t payload[MAX_PAYLOAD];
	uint8_t length;
} message_t;

/*
 * Struct to set and read pins. This can be used for analog and digital writes and reads. For digital writes it is
 * just zeros or ones. For analog writes it is an uint8_t. For reads the value field is the one that is being returned.
 * There is just one struct to keep binary small.
 *
 * The value field is large enough to store a function pointer.
 */
typedef struct {
	uint8_t cmd;
	uint8_t pin;
	uint8_t opcode1;
	uint8_t opcode2;
	uint8_t value;
	uint8_t ack;
	uint32_t callback;
} pin_cmd_t;

/*
 * Struct with data to implement sleep command through coroutines.
 */
typedef struct {
	uint8_t cmd;
	uint16_t period;
	uintptr_t coargs;
} sleep_cmd_t;

const uint8_t MAX_TWI_PAYLOAD = MAX_PAYLOAD - 6;

/*
 * Struct for i2c initialization, writes, and reads.
 */
typedef struct {
	uint8_t cmd;
	uint8_t address;
	uint8_t opcode;
	uint8_t length;
	uint8_t ack;
	uint8_t stop;
	uint8_t buf[MAX_TWI_PAYLOAD];
} twi_cmd_t;

