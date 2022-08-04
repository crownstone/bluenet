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
 * Negative for errors
 */
enum MicroappAck {
	CS_ACK_NONE                      = 0x00, // No meaning

	// Ack requests
	CS_ACK_NO_REQUEST                = 0x01, // Explicitly do not ask for an acknowledgement
	CS_ACK_REQUEST                   = 0x02, // Request for other process (microapp or bluenet) to overwrite this field

	// Ack successful replies (positive)
	CS_ACK_SUCCESS                   = 0x03, // Finished successfully
	CS_ACK_WAIT_FOR_SUCCESS          = 0x04, // So far so good, but not done yet

	// Ack error replies (negative)
	CS_ACK_ERR_NOT_FOUND             = -0x01, // A requested entity could not be found
	CS_ACK_ERR_UNKNOWN_PROTOCOL      = -0x02, // The request cannot be interpreted fully
	CS_ACK_ERR_NO_SPACE              = -0x03, // There is no space to fulfill a request
	CS_ACK_ERR_NOT_IMPLEMENTED       = -0x04, // The request can be interpreted but is not implemented yet
	CS_ACK_ERR_BUSY                  = -0x05, // The request cannot be fulfilled because of other ongoing requests
};

/**
 * The main opcodes for microapp commands.
 */
enum MicroappSdkType {
	CS_MICROAPP_SDK_TYPE_NONE            = 0x00, // No meaning, should not be used
	CS_MICROAPP_SDK_TYPE_LOG             = 0x01, // Microapp logs
	CS_MICROAPP_SDK_TYPE_PIN             = 0x02, // GPIO related
	CS_MICROAPP_SDK_TYPE_SWITCH          = 0x03, // Switch and dimmer commands
	CS_MICROAPP_SDK_TYPE_SERVICE_DATA    = 0x04, // Microapp service data updates
	CS_MICROAPP_SDK_TYPE_TWI             = 0x05, // TWI related
	CS_MICROAPP_SDK_TYPE_BLE             = 0x06, // BLE related (excluding mesh)
	CS_MICROAPP_SDK_TYPE_MESH            = 0x07, // Mesh related
	CS_MICROAPP_SDK_TYPE_POWER_USAGE     = 0x08, // Power usage related
	CS_MICROAPP_SDK_TYPE_PRESENCE        = 0x09, // Presence related
	CS_MICROAPP_SDK_TYPE_CONTROL_COMMAND = 0x0A, // Generic control command according to the control command protocol
	CS_MICROAPP_SDK_TYPE_YIELD           = 0x0B, // Microapp yielding to bluenet without expecting a direct return call, i.e. at the end of setup, loop or during a delay
	CS_MICROAPP_SDK_TYPE_CONTINUE        = 0x0C, // Bluenet calling the microapp without an interrupt, i.e. on a tick or subsequent call
};


/**
 * Indicates the GPIO pins of the hardware. These are mostly for the nRF development kits, since the plugs and builtins have no external GPIOS, buttons or leds
 */
enum MicroappSdkPin {
	CS_MICROAPP_SDK_PIN_GPIO0   = 0x00,
	CS_MICROAPP_SDK_PIN_GPIO1   = 0x01,
	CS_MICROAPP_SDK_PIN_GPIO2   = 0x02,
	CS_MICROAPP_SDK_PIN_GPIO3   = 0x03,
	CS_MICROAPP_SDK_PIN_GPIO4   = 0x04,
	CS_MICROAPP_SDK_PIN_GPIO5   = 0x05,
	CS_MICROAPP_SDK_PIN_GPIO6   = 0x06,
	CS_MICROAPP_SDK_PIN_GPIO7   = 0x07,
	CS_MICROAPP_SDK_PIN_GPIO8   = 0x08,
	CS_MICROAPP_SDK_PIN_GPIO9   = 0x09,
	CS_MICROAPP_SDK_PIN_BUTTON1 = 0x0A,
	CS_MICROAPP_SDK_PIN_BUTTON2 = 0x0B,
	CS_MICROAPP_SDK_PIN_BUTTON3 = 0x0C,
	CS_MICROAPP_SDK_PIN_BUTTON4 = 0x0D,
	CS_MICROAPP_SDK_PIN_LED1    = 0x0E,
	CS_MICROAPP_SDK_PIN_LED2    = 0x0F,
	CS_MICROAPP_SDK_PIN_LED3    = 0x10,
	CS_MICROAPP_SDK_PIN_LED4    = 0x11,
};

/**
 * Indicates whether the pin is to be initialized (MODE) or perform an action (ACTION)
 */
enum MicroappSdkPinType {
	CS_MICROAPP_SDK_PIN_INIT   = 0x01, // Initialize the pin with a polarity and a direction and register an interrupt
	CS_MICROAPP_SDK_PIN_ACTION = 0x02, // An action such as read the value of a pin or write to it
};

/**
 * Directionality of the GPIO pin (input or output)
 */
enum MicroappSdkPinDirection {
	CS_MICROAPP_SDK_PIN_INPUT          = 0x01, // Set pin as input, but do not use a pulling resistor
	CS_MICROAPP_SDK_PIN_INPUT_PULLUP   = 0x02, // Set pin as input using a pullup resistor
	CS_MICROAPP_SDK_PIN_INPUT_PULLDOWN = 0x03, // Set pin as input using a pulldown resistor
	CS_MICROAPP_SDK_PIN_OUTPUT         = 0x04, // Set pin as output
};

/**
 * Polarity of pin for initializing pin interrupts (only for input pins)
 */
enum MicroappSdkPinPolarity {
	CS_MICROAPP_SDK_PIN_CHANGE  = 0x00, // LOTOHI or HITOLO
	CS_MICROAPP_SDK_PIN_RISING  = 0x01, // LOTOHI
	CS_MICROAPP_SDK_PIN_FALLING = 0x02, // HITOLO
};

/**
 * Type of action to perform on a pin, either read or write
 */
enum MicroappSdkPinActionType {
	CS_MICROAPP_SDK_PIN_READ   = 0x00,
	CS_MICROAPP_SDK_PIN_WRITE  = 0x01,
};

/**
 * Value to either read from the pin or write to the pin
 */
enum MicroappSdkPinValue {
	CS_MICROAPP_SDK_PIN_ON   = 0x00,
	CS_MICROAPP_SDK_PIN_OFF  = 0x01,
};

/**
 * Switch value according to same protocol as switch command value over BLE and UART
 * Values between 0 and 100 can be used for dimming
 */
enum MicroappSdkSwitchValue {
	CS_MICROAPP_SDK_SWITCH_OFF       = 0x00, // 0   = fully off
	CS_MICROAPP_SDK_SWITCH_ON        = 0x64, // 100 = fully on
	CS_MICROAPP_SDK_SWITCH_TOGGLE    = 0xFD, // Switch off when currently on, switch to smart on when currently off
	CS_MICROAPP_SDK_SWITCH_BEHAVIOUR = 0xFE, // Switch to the value according to behaviour rules
	CS_MICROAPP_SDK_SWITCH_SMART_ON  = 0xFF, // Switch on, the value will be determined by behaviour rules
};

/**
 * Type of log indicating how to interpret the log payload
 */
enum MicroappSdkLogType {
	CS_MICROAPP_SDK_LOG_CHAR   = 0x00, // Char or byte
	CS_MICROAPP_SDK_LOG_INT    = 0x01, // Signed int (32-bit)
	CS_MICROAPP_SDK_LOG_STR    = 0x02, // String or char array, same as arr
	CS_MICROAPP_SDK_LOG_ARR    = 0x03, // Byte array, same as str
	CS_MICROAPP_SDK_LOG_FLOAT  = 0x04, // Float
	CS_MICROAPP_SDK_LOG_DOUBLE = 0x05, // Double
	CS_MICROAPP_SDK_LOG_UINT   = 0x06, // Unsigned int (32-bit)
	CS_MICROAPP_SDK_LOG_SHORT  = 0x07, // Unsigned short (16-bit)
};

/**
 * Flags for logging. Currently only using a newline flag
 */
enum MicroappSdkLogFlags {
	CS_MICROAPP_SDK_LOG_FLAGS_NEWLINE    = (1 << 0),
};

/**
 * Type of TWI request
 */
enum MicroappSdkTwiType {
	CS_MICROAPP_SDK_TWI_READ    = 0x01,
	CS_MICROAPP_SDK_TWI_WRITE   = 0x02,
	CS_MICROAPP_SDK_TWI_INIT    = 0x03,
	CS_MICROAPP_SDK_TWI_ENABLE  = 0x04,
	CS_MICROAPP_SDK_TWI_DISABLE = 0x05,
};

/**
 * Type of BLE request, indicating how to interpret the rest of the request
 */
enum MicroappSdkBleType {
	CS_MICROAPP_SDK_BLE_SCAN       = 0x01, // Scan-related requests
	CS_MICROAPP_SDK_BLE_CONNECTION = 0x02, // Connection-related requests
};

/**
 * BLE scan request types
 */
enum MicroappSdkBleScanType {
	CS_MICROAPP_SDK_BLE_SCAN_START              = 0x00, // Start forwarding scanned devices to the microapp
	CS_MICROAPP_SDK_BLE_SCAN_STOP               = 0x01, // Stop forwarding scanned devices to the microapp
	CS_MICROAPP_SDK_BLE_SCAN_REGISTER_INTERRUPT = 0x02, // Register an interrupt for incoming scanned devices
};

/**
 * BLE connection request types
 */
enum MicroappSdkBleConnectionType {
	CS_MICROAPP_SDK_BLE_CONNECTION_CONNECT    = 0x00, // Connect to a peripheral
	CS_MICROAPP_SDK_BLE_CONNECTION_DISCONNECT = 0x01, // Disconnect from a peripheral
};

/**
 * Mesh request types
 */
enum CommandMicroappMeshType {
	CS_MICROAPP_COMMAND_MESH_TYPE_SEND         = 0x00, // Send a mesh message
	CS_MICROAPP_COMMAND_MESH_TYPE_LISTEN       = 0x01, // Start listening for mesh messages of the microapp type, and register an interrupt
	CS_MICROAPP_COMMAND_MESH_TYPE_READ_CONFIG  = 0x02, // Request for information about the mesh configuration. At the moment consisting only of the own stone ID
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
 * Struct for commands relating to the dimmer and switch
 * Opcode indicates whether command relates to dimmer or switch
 * Value is a binary value if controlling the switch, or else an unsigned 8-bit intensity value
 */
struct __attribute__((packed)) microapp_dimmer_switch_cmd_t {
	microapp_cmd_t header;
	uint8_t opcode;
	uint8_t value;
};

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
	microapp_mesh_cmd_t meshHeader;
	uint8_t stoneId;  //< Target stone ID, or 0 for broadcast.
	uint8_t dlen;
	uint8_t data[MICROAPP_MAX_MESH_MESSAGE_SIZE];
};

static_assert(sizeof(microapp_mesh_send_cmd_t) <= MAX_PAYLOAD);

/**
 * Struct for microapp mesh read events
 *
 * stoneId 0 is for broadcasted messages
 */
struct __attribute__((packed)) microapp_mesh_read_cmd_t {
	microapp_mesh_cmd_t meshHeader;
	uint8_t stoneId;
	uint8_t dlen;
	uint8_t data[MICROAPP_MAX_MESH_MESSAGE_SIZE];
};

static_assert(sizeof(microapp_mesh_read_cmd_t) <= MAX_PAYLOAD);

/**
 * Struct for microapp mesh info events
 * E.g. used by microapp to get to know its own stone id
 * Can be expanded to include more mesh info
 */
struct __attribute__((packed)) microapp_mesh_info_cmd_t {
	microapp_mesh_cmd_t meshHeader;
	uint8_t stoneId; //< Own stone id
};

static_assert(sizeof(microapp_mesh_info_cmd_t) <= MAX_PAYLOAD);

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
