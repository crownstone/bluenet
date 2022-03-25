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

// This is a given for a MAC address
const uint8_t MAC_ADDRESS_LENGTH = 6;

// This is a given for BLE advertisement data
const uint8_t MAX_BLE_ADV_DATA_LENGTH = 31;

// This is a given for Mesh messages
const uint8_t MICROAPP_MAX_MESH_MESSAGE_SIZE = 7;

// This is the information we want in each command
const uint8_t MICROAPP_CMD_SIZE_HEADER = 6;

// This is the size of a pin command header
const uint8_t MICROAPP_PIN_CMD_SIZE_HEADER = 10;

// Derived maximum payload (should at least fit BLE advertisement data)

const uint8_t MAX_PAYLOAD = 48;

const uint8_t MAX_TWI_PAYLOAD = MAX_PAYLOAD - MICROAPP_CMD_SIZE_HEADER - 5;

// Derived size limitations (change when structs get more info)

const uint8_t MAX_MICROAPP_STRING_LENGTH = MAX_PAYLOAD - MICROAPP_CMD_SIZE_HEADER - 4;

const uint8_t MAX_MICROAPP_ARRAY_LENGTH = MAX_MICROAPP_STRING_LENGTH;

const uint8_t MAX_COMMAND_SERVICE_DATA_LENGTH = MAX_PAYLOAD - MICROAPP_PIN_CMD_SIZE_HEADER - 3;

// Protocol definitions

#define MICROAPP_SERIAL_DEFAULT_PORT_NUMBER 1

#define MICROAPP_SERIAL_SERVICE_DATA_PORT_NUMBER 4

// Call loop every 10 ticks. The ticks are every 100 ms so this means every second.
#define MICROAPP_LOOP_FREQUENCY 10

#ifndef TICK_INTERVAL_MS
#define TICK_INTERVAL_MS 100
#endif

#define MICROAPP_LOOP_INTERVAL_MS (TICK_INTERVAL_MS * MICROAPP_LOOP_FREQUENCY)

/**
 * Arguments for the opcode as first argument in the callback from the microapp to bluenet.
 */
enum CallbackMicroappOpcode {
	CS_MICROAPP_CALLBACK_NONE             = 0x00,
	CS_MICROAPP_CALLBACK_SIGNAL           = 0x01,
	CS_MICROAPP_CALLBACK_UPDATE_IO_BUFFER = 0x02,
};

/**
 * Acknowledgments from microapp to bluenet or the other way around.
 */
enum CommandMicroappAck {
	CS_ACK_NONE                      = 0x00,
	CS_ACK_BLUENET_MICROAPP_REQUEST  = 0x01,
	CS_ACK_BLUENET_MICROAPP_REQ_ACK  = 0x02,
	CS_ACK_BLUENET_MICROAPP_REQ_BUSY = 0x03,
	CS_ACK_MICROAPP_BLUENET_REQUEST  = 0x04,
	CS_ACK_MICROAPP_BLUENET_REQ_ACK  = 0x05,
	CS_ACK_MICROAPP_BLUENET_REQ_BUSY = 0x06,
};

/**
 * The main opcodes for microapp commands.
 */
enum CommandMicroapp {
	CS_MICROAPP_COMMAND_NONE                    = 0x00,
	CS_MICROAPP_COMMAND_LOG                     = 0x01,
	CS_MICROAPP_COMMAND_DELAY                   = 0x02,
	CS_MICROAPP_COMMAND_PIN                     = 0x03,
	CS_MICROAPP_COMMAND_SERVICE_DATA            = 0x04,
	CS_MICROAPP_COMMAND_TWI                     = 0x05,
	CS_MICROAPP_COMMAND_BLE                     = 0x06,
	CS_MICROAPP_COMMAND_POWER_USAGE             = 0x07,
	CS_MICROAPP_COMMAND_PRESENCE                = 0x08,
	CS_MICROAPP_COMMAND_MESH                    = 0x09,
	CS_MICROAPP_COMMAND_SETUP_END               = 0x0A,
	CS_MICROAPP_COMMAND_LOOP_END                = 0x0B,
	CS_MICROAPP_COMMAND_BLE_DEVICE              = 0x0C,
	CS_MICROAPP_COMMAND_SOFT_INTERRUPT_RECEIVED = 0x0D,
	CS_MICROAPP_COMMAND_SOFT_INTERRUPT_END      = 0x0E,
	CS_MICROAPP_COMMAND_SOFT_INTERRUPT_ERROR    = 0x0F,
};

enum CommandMicroappPin {
	CS_MICROAPP_COMMAND_PIN_SWITCH  = 0x00,  // Virtual pin
	CS_MICROAPP_COMMAND_PIN_DIMMER  = 0x00,  // same one
	CS_MICROAPP_COMMAND_PIN_GPIO1   = 0x01,  // GPIO pin on connector
	CS_MICROAPP_COMMAND_PIN_GPIO2   = 0x02,  // GPIO pin on connector
	CS_MICROAPP_COMMAND_PIN_GPIO3   = 0x03,  // GPIO pin on connector
	CS_MICROAPP_COMMAND_PIN_GPIO4   = 0x04,  // GPIO pin on connector
	CS_MICROAPP_COMMAND_PIN_BUTTON1 = 0x05,  // BUTTON1
	CS_MICROAPP_COMMAND_PIN_BUTTON2 = 0x06,  // BUTTON2
	CS_MICROAPP_COMMAND_PIN_BUTTON3 = 0x07,  // BUTTON3
	CS_MICROAPP_COMMAND_PIN_BUTTON4 = 0x08,  // BUTTON4
	CS_MICROAPP_COMMAND_PIN_LED1    = 0x09,  // LED1
	CS_MICROAPP_COMMAND_PIN_LED2    = 0x0a,  // LED2
	CS_MICROAPP_COMMAND_PIN_LED3    = 0x0b,  // LED3
	CS_MICROAPP_COMMAND_PIN_LED4    = 0x0c,  // LED4
};

enum CommandMicroappPinOpcode1 {
	CS_MICROAPP_COMMAND_PIN_MODE   = 0x00,
	CS_MICROAPP_COMMAND_PIN_ACTION = 0x01,
};

enum CommandMicroappPinOpcode2 {
	CS_MICROAPP_COMMAND_PIN_READ         = 0x01,
	CS_MICROAPP_COMMAND_PIN_WRITE        = 0x02,
	CS_MICROAPP_COMMAND_PIN_TOGGLE       = 0x03,
	CS_MICROAPP_COMMAND_PIN_INPUT_PULLUP = 0x04,
};

enum CommandMicroappPinValue {
	CS_MICROAPP_COMMAND_VALUE_OFF     = 0x00,
	CS_MICROAPP_COMMAND_VALUE_ON      = 0x01,
	CS_MICROAPP_COMMAND_VALUE_CHANGE  = 0x02,
	CS_MICROAPP_COMMAND_VALUE_RISING  = 0x03,
	CS_MICROAPP_COMMAND_VALUE_FALLING = 0x04,
};

enum CommandMicroappLog {
	CS_MICROAPP_COMMAND_LOG_CHAR   = 0x00,
	CS_MICROAPP_COMMAND_LOG_INT    = 0x01,
	CS_MICROAPP_COMMAND_LOG_STR    = 0x02,
	CS_MICROAPP_COMMAND_LOG_ARR    = 0x03,
	CS_MICROAPP_COMMAND_LOG_FLOAT  = 0x04,
	CS_MICROAPP_COMMAND_LOG_DOUBLE = 0x05,
	CS_MICROAPP_COMMAND_LOG_UINT   = 0x06,
	CS_MICROAPP_COMMAND_LOG_SHORT  = 0x07,
};

enum CommandMicroappLogOption {
	CS_MICROAPP_COMMAND_LOG_NEWLINE    = 0x00,
	CS_MICROAPP_COMMAND_LOG_NO_NEWLINE = 0x01,
};

enum CommandMicroappTwiOpcode {
	CS_MICROAPP_COMMAND_TWI_READ    = 0x01,
	CS_MICROAPP_COMMAND_TWI_WRITE   = 0x02,
	CS_MICROAPP_COMMAND_TWI_INIT    = 0x03,
	CS_MICROAPP_COMMAND_TWI_ENABLE  = 0x04,
	CS_MICROAPP_COMMAND_TWI_DISABLE = 0x05,
};

enum CommandMicroappBleOpcode {
	CS_MICROAPP_COMMAND_BLE_SCAN_SET_HANDLER = 0x01,
	CS_MICROAPP_COMMAND_BLE_SCAN_START       = 0x02,
	CS_MICROAPP_COMMAND_BLE_SCAN_STOP        = 0x03,
	CS_MICROAPP_COMMAND_BLE_CONNECT          = 0x04,
};

enum CommandMicroappMeshOpcode {
	CS_MICROAPP_COMMAND_MESH_SEND           = 0x00,
	CS_MICROAPP_COMMAND_MESH_READ_AVAILABLE = 0x01,
	CS_MICROAPP_COMMAND_MESH_READ           = 0x02,
};

enum MicroappBleEventType {
	BleEventDeviceScanned = 0x01,
	BleEventConnected     = 0x02,
	BleEventDisconnected  = 0x03,
};

/**
 * A single buffer (can be either input or output).
 */
struct __attribute__((packed)) io_buffer_t {
	uint8_t payload[MAX_PAYLOAD];
};

/**
 * Combined input and output buffer.
 */
struct __attribute__((packed)) bluenet_io_buffer_t {
	io_buffer_t microapp2bluenet;
	io_buffer_t bluenet2microapp;
};

typedef uint16_t (*microappCallbackFunc)(uint8_t opcode, bluenet_io_buffer_t*);

/*
 * The layout of the struct in ramdata. We set for the microapp a protocol version so it can check itself if it is
 * compatible. The length parameter functions as a extra possible check. The callback can be used by the microapp to
 * call back into bluenet. The pointer to the coargs struct can be used to switch back from the used coroutine and
 * needs to stored somewhere accessible.
 */
struct __attribute__((packed)) bluenet2microapp_ipcdata_t {
	uint8_t protocol;
	uint8_t length;
	microappCallbackFunc microappCallback;
	bool valid;
};

/**
 * A payload has always a cmd field as opcode. The id can be used for identification, for example useful for soft
 * interrupts. The ack field can also be used for interrupts.
 */
struct __attribute__((packed)) microapp_cmd_t {
	uint8_t cmd;
	uint8_t id;
	uint8_t ack;
	uint8_t prev;
	uint8_t interruptCmd;
	uint8_t counter;
};

static_assert(sizeof(microapp_cmd_t) == MICROAPP_CMD_SIZE_HEADER);

/*
 * Struct to set and read pins. This can be used for analog and digital writes and reads. For digital writes it is
 * just zeros or ones. For analog writes it is an uint8_t. For reads the value field is the one that is being returned.
 * There is just one struct to keep binary small.
 *
 * The value field is large enough to store a function pointer.
 */
struct __attribute__((packed)) microapp_pin_cmd_t {
	microapp_cmd_t header;
	uint8_t pin;      // CommandMicroappPin
	uint8_t opcode1;  // CommandMicroappPinOpcode1
	uint8_t opcode2;  // CommandMicroappPinOpcode2
	uint8_t value;    // CommandMicroappPinValue
};

static_assert(sizeof(microapp_pin_cmd_t) == MICROAPP_PIN_CMD_SIZE_HEADER);
static_assert(sizeof(microapp_pin_cmd_t) <= MAX_PAYLOAD);

/*
 * Struct with data to implement delay command through coroutines.
 */
struct __attribute__((packed)) microapp_delay_cmd_t {
	microapp_cmd_t header;
	uint16_t period;
	uintptr_t coargs;
};

static_assert(sizeof(microapp_delay_cmd_t) <= MAX_PAYLOAD);

/*
 * Struct for i2c initialization, writes, and reads.
 */
struct __attribute__((packed)) microapp_twi_cmd_t {
	microapp_cmd_t header;
	uint8_t address;
	uint8_t opcode;
	uint8_t length;
	uint8_t stop;
	uint8_t buf[MAX_TWI_PAYLOAD];
};

static_assert(sizeof(microapp_twi_cmd_t) <= MAX_PAYLOAD);

/*
 * Struct for log commands
 */
struct __attribute__((packed)) microapp_log_cmd_t {
	microapp_cmd_t header;
	uint8_t port;
	uint8_t type;
	uint8_t option;
	uint8_t length;
};

static_assert(sizeof(microapp_log_cmd_t) <= MAX_PAYLOAD);

struct __attribute__((packed)) microapp_log_char_cmd_t {
	microapp_log_cmd_t log_header;
	uint8_t value;
};

struct __attribute__((packed)) microapp_log_short_cmd_t {
	microapp_log_cmd_t log_header;
	uint16_t value;
};

struct __attribute__((packed)) microapp_log_uint_cmd_t {
	microapp_log_cmd_t log_header;
	uint32_t value;
};

struct __attribute__((packed)) microapp_log_int_cmd_t {
	microapp_log_cmd_t log_header;
	int32_t value;
};

struct __attribute__((packed)) microapp_log_float_cmd_t {
	microapp_log_cmd_t log_header;
	float value;
};

struct __attribute__((packed)) microapp_log_double_cmd_t {
	microapp_log_cmd_t log_header;
	double value;
};

struct __attribute__((packed)) microapp_log_string_cmd_t {
	microapp_log_cmd_t log_header;
	char str[MAX_MICROAPP_STRING_LENGTH];
};

static_assert(sizeof(microapp_log_string_cmd_t) <= MAX_PAYLOAD);

struct __attribute__((packed)) microapp_log_array_cmd_t {
	microapp_log_cmd_t log_header;
	char arr[MAX_MICROAPP_ARRAY_LENGTH];
};

/*
 * Struct for microapp ble commands
 */
struct __attribute__((packed)) microapp_ble_cmd_t {
	microapp_cmd_t header;
	uint8_t opcode;
	uint8_t id;
	uint8_t addr[MAC_ADDRESS_LENGTH];  // big-endian!
};

static_assert(sizeof(microapp_ble_cmd_t) <= MAX_PAYLOAD);

/**
 * Struct for mesh message from microapp.
 */
struct __attribute__((packed)) microapp_mesh_cmd_t {
	microapp_cmd_t header;
	uint8_t opcode;
};

static_assert(sizeof(microapp_mesh_cmd_t) <= MAX_PAYLOAD);

/*
 * Struct for header of microapp mesh send commands
 */
struct __attribute__((packed)) microapp_mesh_send_cmd_t {
	struct microapp_mesh_cmd_t mesh_header;
	uint8_t stoneId;  //< Target stone ID, or 0 for broadcast.
	uint8_t dlen;
	uint8_t data[MICROAPP_MAX_MESH_MESSAGE_SIZE];
};

static_assert(sizeof(microapp_mesh_send_cmd_t) <= MAX_PAYLOAD);

struct __attribute__((packed)) microapp_mesh_read_available_cmd_t {
	struct microapp_mesh_cmd_t mesh_header;
	uint8_t available;
};

static_assert(sizeof(microapp_mesh_read_available_cmd_t) <= MAX_PAYLOAD);

/**
 * Struct for microapp mesh read commands
 *
 * stoneId 0 is for broadcasted messages
 */
struct __attribute__((packed)) microapp_mesh_read_cmd_t {
	microapp_mesh_cmd_t mesh_header;
	uint8_t stoneId;
	uint8_t dlen;
	uint8_t data[MICROAPP_MAX_MESH_MESSAGE_SIZE];
};

static_assert(sizeof(microapp_mesh_read_cmd_t) <= MAX_PAYLOAD);

/*
 * Struct for scanned ble devices sent to the microapp
 */
struct __attribute__((packed)) microapp_ble_device_t {
	microapp_cmd_t header;
	uint8_t type;
	uint8_t addr_type;
	uint8_t addr[MAC_ADDRESS_LENGTH];  // big-endian!
	int8_t rssi;
	uint8_t dlen;
	uint8_t data[MAX_BLE_ADV_DATA_LENGTH];
};

static_assert(sizeof(microapp_ble_device_t) <= MAX_PAYLOAD);

/**
 * Struct for microapp service data itself.
 */
struct __attribute__((packed)) microapp_service_data_t {
	uint16_t appUuid;
	uint8_t dlen;
	uint8_t data[MAX_COMMAND_SERVICE_DATA_LENGTH];
};

/**
 * Struct for microapp service data commands
 *
 * This reuses the log_cmd format.
 */
struct __attribute__((packed)) microapp_service_data_cmd_t {
	microapp_log_cmd_t log_header;
	microapp_service_data_t body;
};

static_assert(sizeof(microapp_service_data_cmd_t) <= MAX_PAYLOAD);

/*
 * Struct for microapp power usage requests
 */
struct __attribute__((packed)) microapp_power_usage_t {
	int32_t powerUsage;
};

struct __attribute__((packed)) microapp_power_usage_cmd_t {
	microapp_cmd_t header;
	microapp_power_usage_t powerUsage;
};

/*
 * Struct for microapp presence requests
 */
struct __attribute__((packed)) microapp_presence_t {
	uint8_t profileId;
	uint64_t presenceBitmask;
};

struct __attribute__((packed)) microapp_presence_cmd_t {
	microapp_cmd_t header;
	microapp_presence_t presence;
};

struct __attribute__((packed)) microapp_soft_interrupt_cmd_t {
	microapp_cmd_t header;
	uint8_t emptyInterruptSlots;
};

static_assert(sizeof(microapp_soft_interrupt_cmd_t) <= MAX_PAYLOAD);
