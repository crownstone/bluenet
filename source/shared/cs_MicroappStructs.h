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

// Size limitations

const uint8_t MAX_PAYLOAD = 44;

const uint8_t MAX_TWI_PAYLOAD = MAX_PAYLOAD - 6;

const uint8_t MAC_ADDRESS_LENGTH = 6;

const uint8_t MAX_BLE_ADV_DATA_LENGTH = 31;

const uint8_t MAX_MICROAPP_STRING_LENGTH = MAX_PAYLOAD - 5;

const uint8_t MAX_MICROAPP_ARRAY_LENGTH = MAX_MICROAPP_STRING_LENGTH;

const uint8_t MAX_MESH_DATA_LENGTH = MAX_PAYLOAD - 4;

const uint8_t MICROAPP_MAX_MESH_MESSAGE_SIZE = 7;

const uint8_t MAX_COMMAND_SERVICE_DATA_LENGTH = MAX_PAYLOAD - 8;

// Protocol definitions

#define MICROAPP_SERIAL_DEFAULT_PORT_NUMBER 1

#define MICROAPP_SERIAL_SERVICE_DATA_PORT_NUMBER 4

/**
 * The main opcodes for microapp commands.
 */
enum CommandMicroapp {
	CS_MICROAPP_COMMAND_NONE          = 0x00,
	CS_MICROAPP_COMMAND_LOG           = 0x01,
	CS_MICROAPP_COMMAND_DELAY         = 0x02,
	CS_MICROAPP_COMMAND_PIN           = 0x03,  // Payload is pin_cmd_t.
	CS_MICROAPP_COMMAND_SERVICE_DATA  = 0x04,
	CS_MICROAPP_COMMAND_TWI           = 0x05,
	CS_MICROAPP_COMMAND_BLE           = 0x06,
	CS_MICROAPP_COMMAND_POWER_USAGE   = 0x07,
	CS_MICROAPP_COMMAND_PRESENCE      = 0x08,
	CS_MICROAPP_COMMAND_MESH          = 0x09,
	CS_MICROAPP_COMMAND_SETUP_END     = 0x0A,
	CS_MICROAPP_COMMAND_LOOP_END      = 0x0B,
	CS_MICROAPP_COMMAND_BLE_DEVICE    = 0x0C,
	CS_MICROAPP_COMMAND_CALLBACK_DONE = 0x0D,
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

/*
 * The layout of the struct in ramdata. We set for the microapp a protocol version so it can check itself if it is
 * compatible. The length parameter functions as a extra possible check. The callback can be used by the microapp to
 * call back into bluenet. The pointer to the coargs struct can be used to switch back from the used coroutine and
 * needs to stored somewhere accessible.
 */
struct __attribute__((packed)) bluenet2microapp_ipcdata_t {
	uint8_t protocol;
	uint8_t length;
	uintptr_t microapp_callback;
	uintptr_t coargs_ptr;
	bool valid;
};

/*
 * The layout of the struct in ramdata from microapp to bluenet.
 */
struct __attribute__((packed)) microapp2bluenet_ipcdata_t {
	uint8_t protocol;
	uint8_t length;
};

/**
 * A payload has always a cmd field as opcode. The id can be used for identification, for example useful for callbacks.
 * The ack field can be used for callbacks.
 */
struct __attribute__((packed)) microapp_cmd_t {
	uint8_t cmd;
	uint8_t id;
	uint8_t ack;
};

/**
 * An acked payload can be used for a callback.
 */
struct __attribute__((packed)) microapp_acked_cmd_t {
	uint8_t cmd;
	uint8_t ack;
};

/*
 * Struct to set and read pins. This can be used for analog and digital writes and reads. For digital writes it is
 * just zeros or ones. For analog writes it is an uint8_t. For reads the value field is the one that is being returned.
 * There is just one struct to keep binary small.
 *
 * The value field is large enough to store a function pointer.
 */
struct __attribute__((packed)) microapp_pin_cmd_t {
	uint8_t cmd = CS_MICROAPP_COMMAND_PIN;
	uint8_t ack;
	uint8_t pin;      // CommandMicroappPin
	uint8_t opcode1;  // CommandMicroappPinOpcode1
	uint8_t opcode2;  // CommandMicroappPinOpcode2
	uint8_t value;    // CommandMicroappPinValue
};

static_assert(sizeof(microapp_pin_cmd_t) <= MAX_PAYLOAD);

/*
 * Struct with data to implement delay command through coroutines.
 */
struct __attribute__((packed)) microapp_delay_cmd_t {
	uint8_t cmd = CS_MICROAPP_COMMAND_DELAY;
	uint16_t period;
	uintptr_t coargs;
};

static_assert(sizeof(microapp_delay_cmd_t) <= MAX_PAYLOAD);

/*
 * Struct for i2c initialization, writes, and reads.
 */
struct __attribute__((packed)) microapp_twi_cmd_t {
	uint8_t cmd = CS_MICROAPP_COMMAND_TWI;
	uint8_t ack;
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
	uint8_t cmd = CS_MICROAPP_COMMAND_LOG;
	uint8_t port;
	uint8_t type;
	uint8_t option;
	uint8_t length;
};

static_assert(sizeof(microapp_log_cmd_t) <= MAX_PAYLOAD);

struct __attribute__((packed)) microapp_log_char_cmd_t {
	microapp_log_cmd_t header;
	uint8_t value;
};

struct __attribute__((packed)) microapp_log_short_cmd_t {
	microapp_log_cmd_t header;
	uint16_t value;
};

struct __attribute__((packed)) microapp_log_uint_cmd_t {
	microapp_log_cmd_t header;
	uint32_t value;
};

struct __attribute__((packed)) microapp_log_int_cmd_t {
	microapp_log_cmd_t header;
	int32_t value;
};

struct __attribute__((packed)) microapp_log_float_cmd_t {
	microapp_log_cmd_t header;
	float value;
};

struct __attribute__((packed)) microapp_log_double_cmd_t {
	microapp_log_cmd_t header;
	double value;
};

struct __attribute__((packed)) microapp_log_string_cmd_t {
	microapp_log_cmd_t header;
	char str[MAX_MICROAPP_STRING_LENGTH];
};

static_assert(sizeof(microapp_log_string_cmd_t) <= MAX_PAYLOAD);

struct __attribute__((packed)) microapp_log_array_cmd_t {
	microapp_log_cmd_t header;
	char arr[MAX_MICROAPP_ARRAY_LENGTH];
};

/*
 * Struct for microapp ble commands
 */
struct __attribute__((packed)) microapp_ble_cmd_t {
	uint8_t cmd = CS_MICROAPP_COMMAND_BLE;
	uint8_t ack;
	uint8_t opcode;
	uint8_t id;
};

static_assert(sizeof(microapp_ble_cmd_t) <= MAX_PAYLOAD);

/**
 * Struct for mesh message from microapp.
 */
struct __attribute__((packed)) microapp_mesh_cmd_t {
	uint8_t cmd = CS_MICROAPP_COMMAND_MESH;
	uint8_t opcode;
};

static_assert(sizeof(microapp_mesh_cmd_t) <= MAX_PAYLOAD);

/*
 * Struct for header of microapp mesh send commands
 */
struct __attribute__((packed)) microapp_mesh_send_cmd_t {
	struct microapp_mesh_cmd_t header;
	uint8_t stoneId;  //< Target stone ID, or 0 for broadcast.
	uint8_t dlen;
	uint8_t data[MICROAPP_MAX_MESH_MESSAGE_SIZE];
};

static_assert(sizeof(microapp_mesh_send_cmd_t) <= MAX_PAYLOAD);

struct __attribute__((packed)) microapp_mesh_read_available_cmd_t {
	struct microapp_mesh_cmd_t header;
	uint8_t available;
};

static_assert(sizeof(microapp_mesh_read_available_cmd_t) <= MAX_PAYLOAD);

/**
 * Struct for microapp mesh read commands
 */
struct __attribute__((packed)) microapp_mesh_read_cmd_t {
	struct microapp_mesh_cmd_t header;
	uint8_t stoneId;                                  // Target stone ID, or 0 for broadcast.
	uint8_t messageSize;                              // Actual message size.
	uint8_t message[MICROAPP_MAX_MESH_MESSAGE_SIZE];  // Message buffer.
};

static_assert(sizeof(microapp_mesh_read_cmd_t) <= MAX_PAYLOAD);

/*
 * Struct for scanned ble devices sent to the microapp
 */
struct __attribute__((packed)) microapp_ble_device_t {
	uint8_t cmd = CS_MICROAPP_COMMAND_BLE_DEVICE;
	uint8_t ack;
	uint8_t id;
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
	microapp_log_cmd_t header;
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
	uint8_t cmd = CS_MICROAPP_COMMAND_POWER_USAGE;
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
	uint8_t cmd = CS_MICROAPP_COMMAND_PRESENCE;
	microapp_presence_t presence;
};
