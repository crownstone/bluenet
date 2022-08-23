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

/*
 * Externally determined constant sizes
 */

// Standard MAC address length
const uint8_t MAC_ADDRESS_LENGTH                            = 6;
// Defined by the BLE SIG
const uint8_t MAX_BLE_ADV_DATA_LENGTH                       = 31;
// Defined by the mesh protocol
const uint8_t MAX_MICROAPP_MESH_PAYLOAD_SIZE                = 7;
// Defined by the service data packet service_data_encrypted_microapp_t
const uint8_t MICROAPP_SDK_MAX_SERVICE_DATA_LENGTH          = 8;
/*
 * Payload and header sizes (may need to be updated when structs are changed)
 */

// Maximum total payload (somewhat arbitrary, should be able to contain most-used data structures e.g. BLE
// advertisements)
const uint8_t MICROAPP_SDK_MAX_PAYLOAD                      = 48;
// messageType [1] + ack [1]
const uint8_t MICROAPP_SDK_HEADER_SIZE                      = 2;
// header + type [1] + flags [1] + size [1]
const uint8_t MICROAPP_SDK_LOG_HEADER_SIZE                  = MICROAPP_SDK_HEADER_SIZE + 3;
// max total - (header + twiType [1] + twiAddress [1] + twiFlags [1] + twiPayloadSize [1])
const uint8_t MICROAPP_SDK_MAX_TWI_PAYLOAD_SIZE             = MICROAPP_SDK_MAX_PAYLOAD - (MICROAPP_SDK_HEADER_SIZE + 4);
// max total - log header
const uint8_t MICROAPP_SDK_MAX_STRING_LENGTH                = MICROAPP_SDK_MAX_PAYLOAD - MICROAPP_SDK_LOG_HEADER_SIZE;
// max total - log header
const uint8_t MICROAPP_SDK_MAX_ARRAY_SIZE                   = MICROAPP_SDK_MAX_PAYLOAD - MICROAPP_SDK_LOG_HEADER_SIZE;
// max total - (header + protocol [1] + type [2] + size [2])
const uint8_t MICROAPP_SDK_MAX_CONTROL_COMMAND_PAYLOAD_SIZE = MICROAPP_SDK_MAX_PAYLOAD - (MICROAPP_SDK_HEADER_SIZE + 5);

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
enum MicroappSdkAck {
	// Ack successfull return value
	CS_MICROAPP_SDK_ACK_SUCCESS             = 0x00,  // Finished successfully

	// Ack requests (should not be interpreted as a return value)
	CS_MICROAPP_SDK_ACK_NO_REQUEST          = 0x01,  // Explicitly do not ask for an acknowledgement
	CS_MICROAPP_SDK_ACK_REQUEST             = 0x02,  // Request for other process (microapp or bluenet) to overwrite this field

	// Ack return values
	CS_MICROAPP_SDK_ACK_IN_PROGRESS         = 0x03,  // So far so good, but not done yet
	CS_MICROAPP_SDK_ACK_ERROR               = 0x04,  // Unspecified error
	CS_MICROAPP_SDK_ACK_ERR_NOT_FOUND       = 0x05,  // A requested entity could not be found
	CS_MICROAPP_SDK_ACK_ERR_UNDEFINED       = 0x06,  // The request cannot be interpreted fully
	CS_MICROAPP_SDK_ACK_ERR_NO_SPACE        = 0x07,  // There is no space to fulfill a request
	CS_MICROAPP_SDK_ACK_ERR_NOT_IMPLEMENTED = 0x08,  // The request can be interpreted but is not implemented yet
	CS_MICROAPP_SDK_ACK_ERR_BUSY            = 0x09,  // The request cannot be fulfilled because of other ongoing requests
	CS_MICROAPP_SDK_ACK_ERR_OUT_OF_RANGE    = 0x0A,  // A parameter in the request is out of range
	CS_MICROAPP_SDK_ACK_ERR_DISABLED        = 0x0B,  // Request requires functionality that is disabled
	CS_MICROAPP_SDK_ACK_ERR_EMPTY           = 0x0C,  // Request or its parameters are empty
	CS_MICROAPP_SDK_ACK_ERR_TOO_LARGE       = 0x0D,  // Request or its parameters are too large
};

typedef MicroappSdkAck microapp_sdk_result_t;

/**
 * The main opcodes for microapp commands.
 */
enum MicroappSdkMessageType {
	CS_MICROAPP_SDK_TYPE_NONE            = 0x00,  // No meaning, should not be used
	CS_MICROAPP_SDK_TYPE_LOG             = 0x01,  // Microapp logs
	CS_MICROAPP_SDK_TYPE_PIN             = 0x02,  // GPIO related
	CS_MICROAPP_SDK_TYPE_SWITCH          = 0x03,  // Switch and dimmer commands
	CS_MICROAPP_SDK_TYPE_SERVICE_DATA    = 0x04,  // Microapp service data updates
	CS_MICROAPP_SDK_TYPE_TWI             = 0x05,  // TWI related
	CS_MICROAPP_SDK_TYPE_BLE             = 0x06,  // BLE related (excluding mesh)
	CS_MICROAPP_SDK_TYPE_MESH            = 0x07,  // Mesh related
	CS_MICROAPP_SDK_TYPE_POWER_USAGE     = 0x08,  // Power usage related
	CS_MICROAPP_SDK_TYPE_PRESENCE        = 0x09,  // Presence related
	CS_MICROAPP_SDK_TYPE_CONTROL_COMMAND = 0x0A,  // Generic control command according to the control command protocol
	CS_MICROAPP_SDK_TYPE_YIELD           = 0x0B,  // Microapp yielding to bluenet without expecting a direct return call
	CS_MICROAPP_SDK_TYPE_CONTINUE        = 0x0C,  // Bluenet calling the microapp on a tick or subsequent call
};

/**
 * Type of log indicating how to interpret the log payload
 */
enum MicroappSdkLogType {
	CS_MICROAPP_SDK_LOG_CHAR   = 0x01,  // Char or byte
	CS_MICROAPP_SDK_LOG_INT    = 0x02,  // Signed int (32-bit)
	CS_MICROAPP_SDK_LOG_STR    = 0x03,  // String or char array, same as arr
	CS_MICROAPP_SDK_LOG_ARR    = 0x04,  // Byte array, same as str
	CS_MICROAPP_SDK_LOG_FLOAT  = 0x05,  // Float
	CS_MICROAPP_SDK_LOG_DOUBLE = 0x06,  // Double
	CS_MICROAPP_SDK_LOG_UINT   = 0x07,  // Unsigned int (32-bit)
	CS_MICROAPP_SDK_LOG_SHORT  = 0x08,  // Unsigned short (16-bit)
};

/**
 * Flags for logging. Currently only using a newline flag
 */
enum MicroappSdkLogFlags {
	CS_MICROAPP_SDK_LOG_FLAG_CLEAR   = 0,
	CS_MICROAPP_SDK_LOG_FLAG_NEWLINE = (1 << 0),  // Add a newline character
};

/**
 * Indicates the GPIO pins of the hardware.
 * Pin functionality can be used for crownstones that have exposed GPIO pins only
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
	CS_MICROAPP_SDK_PIN_INIT   = 0x01,  // Initialize the pin with a polarity and a direction and register an interrupt
	CS_MICROAPP_SDK_PIN_ACTION = 0x02,  // An action such as read the value of a pin or write to it
};

/**
 * Directionality of the GPIO pin (input or output)
 */
enum MicroappSdkPinDirection {
	CS_MICROAPP_SDK_PIN_INPUT        = 0x01,  // Set pin as input, but do not use a pulling resistor
	CS_MICROAPP_SDK_PIN_INPUT_PULLUP = 0x02,  // Set pin as input using a pullup resistor
	CS_MICROAPP_SDK_PIN_OUTPUT       = 0x03,  // Set pin as output
};

/**
 * Polarity of pin for initializing pin interrupts (only for input pins)
 */
enum MicroappSdkPinPolarity {
	CS_MICROAPP_SDK_PIN_NO_POLARITY = 0x01,  // Not sensing for a specific event
	CS_MICROAPP_SDK_PIN_CHANGE      = 0x02,  // LOTOHI or HITOLO
	CS_MICROAPP_SDK_PIN_RISING      = 0x03,  // LOTOHI
	CS_MICROAPP_SDK_PIN_FALLING     = 0x04,  // HITOLO
};

/**
 * Type of action to perform on a pin, either read or write
 */
enum MicroappSdkPinActionType {
	CS_MICROAPP_SDK_PIN_READ  = 0x01,
	CS_MICROAPP_SDK_PIN_WRITE = 0x02,
};

/**
 * Value to either read from the pin or write to the pin
 */
enum MicroappSdkPinValue {
	CS_MICROAPP_SDK_PIN_OFF = 0x00,
	CS_MICROAPP_SDK_PIN_ON  = 0x01,
};

/**
 * Switch value according to same protocol as switch command value over BLE and UART
 * Values between 0 and 100 can be used for dimming
 */
enum MicroappSdkSwitchValue {
	CS_MICROAPP_SDK_SWITCH_OFF       = 0x00,  // 0   = fully off
	CS_MICROAPP_SDK_SWITCH_ON        = 0x64,  // 100 = fully on
	CS_MICROAPP_SDK_SWITCH_TOGGLE    = 0xFD,  // Switch off when currently on, switch to smart on when currently off
	CS_MICROAPP_SDK_SWITCH_BEHAVIOUR = 0xFE,  // Switch to the value according to behaviour rules
	CS_MICROAPP_SDK_SWITCH_SMART_ON  = 0xFF,  // Switch on, the value will be determined by behaviour rules
};

/**
 * Type of TWI request
 */
enum MicroappSdkTwiType {
	CS_MICROAPP_SDK_TWI_READ  = 0x01,
	CS_MICROAPP_SDK_TWI_WRITE = 0x02,
	CS_MICROAPP_SDK_TWI_INIT  = 0x03,
};

/**
 * Flags for TWI requests
 */
enum MicroappSdkTwiFlags {
	CS_MICROAPP_SDK_TWI_FLAG_CLEAR = 0,
	CS_MICROAPP_SDK_TWI_FLAG_STOP  = (1 << 0),  // Stop bit
};

/**
 * Type of BLE request, indicating how to interpret the rest of the request
 */
enum MicroappSdkBleType {
	CS_MICROAPP_SDK_BLE_NONE                          = 0x00,  // Invalid type
	// Scan related message types
	CS_MICROAPP_SDK_BLE_SCAN_START                    = 0x01,  // Start forwarding scanned devices to the microapp
	CS_MICROAPP_SDK_BLE_SCAN_STOP                     = 0x02,  // Stop forwarding scanned devices to the microapp
	CS_MICROAPP_SDK_BLE_SCAN_REGISTER_INTERRUPT       = 0x03,  // Register an interrupt for incoming scanned devices
	CS_MICROAPP_SDK_BLE_SCAN_SCANNED_DEVICE           = 0x04,  // Bluenet has scanned a device. Used for interrupts
	// Connection related message types
	CS_MICROAPP_SDK_BLE_CONNECTION_REQUEST_CONNECT    = 0x05,  // Request a connection to a peripheral
	CS_MICROAPP_SDK_BLE_CONNECTION_CONNECTED          = 0x06,  // Bluenet -> microapp when connected to a peripheral
	CS_MICROAPP_SDK_BLE_CONNECTION_REQUEST_DISCONNECT = 0x07,  // Request disconnecting from a peripheral
	CS_MICROAPP_SDK_BLE_CONNECTION_DISCONNECTED = 0x08,  // Bluenet -> microapp when disconnected from a peripheral
};

/**
 * Mesh request types
 */
enum MicroappSdkMeshType {
	CS_MICROAPP_SDK_MESH_SEND        = 0x01,  // Send a mesh message from the microapp
	CS_MICROAPP_SDK_MESH_LISTEN      = 0x02,  // Start listening for mesh messages of the microapp type, and register an
											  // interrupt on the bluenet side
	CS_MICROAPP_SDK_MESH_READ_CONFIG = 0x03,  // Request for information about the mesh configuration. At the moment
											  // consisting only of the own stone ID
	CS_MICROAPP_SDK_MESH_READ        = 0x04,  // Received a mesh message. Used for interrupts from bluenet
};

/**
 * Types of power usage to reqeust
 */
enum MicroappSdkPowerUsageType {
	CS_MICROAPP_SDK_POWER_USAGE_POWER   = 0x01,  // Get filtered power data in milliWatt
	CS_MICROAPP_SDK_POWER_USAGE_CURRENT = 0x02,  // Not implemented yet
	CS_MICROAPP_SDK_POWER_USAGE_VOLTAGE = 0x03,  // Not implemented yet
};

/**
 * Type of yield from the microapp to bluenet
 */
enum MicroappSdkYieldType {
	CS_MICROAPP_SDK_YIELD_SETUP = 0x01,  // End of setup
	CS_MICROAPP_SDK_YIELD_LOOP  = 0x02,  // End of loop
	CS_MICROAPP_SDK_YIELD_ASYNC = 0x03,  // The microapp is doing something asynchronous like a delay call
};

/**
 * A single buffer (can be either input or output).
 */
struct __attribute__((packed)) io_buffer_t {
	uint8_t payload[MICROAPP_SDK_MAX_PAYLOAD];
};

/**
 * Combined input and output buffer.
 */
struct __attribute__((packed)) bluenet_io_buffers_t {
	io_buffer_t microapp2bluenet;
	io_buffer_t bluenet2microapp;
};

typedef microapp_sdk_result_t (*microappCallbackFunc)(uint8_t opcode, bluenet_io_buffers_t*);

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
 * @struct microapp_sdk_header_t
 * Header for io buffers shared between bluenet and microapp. The payload of the io buffer always starts with this
 * header.
 *
 * @var microapp_sdk_header_t::messageType  MicroappSdkMessageType     Specifies the type of message, and how to
 * interpret the rest of the payload
 * @var microapp_sdk_header_t::ack          MicroappSdkAck      Used for requesting and receiving acks. Can be used for
 * identifying requests and interrupts.
 */
struct __attribute__((packed)) microapp_sdk_header_t {
	uint8_t messageType;
	int8_t ack;
};

static_assert(sizeof(microapp_sdk_header_t) == MICROAPP_SDK_HEADER_SIZE);

/**
 * @struct microapp_sdk_log_header_t
 * Header for log commands. Excludes the actual log payload, which is different for every log type.
 *
 * @var microapp_sdk_log_header_t::header
 * @var microapp_sdk_log_header_t::type    MicroappSdkLogType   Specifies what type of payload it carries
 * @var microapp_sdk_log_header_t::flags   MicroappSdkLogFlags  Flags for logging. Currently only contains a newline
 * flag
 * @var microapp_sdk_log_header_t::size                         Length of the payload for type STR or ARR
 */
struct __attribute__((packed)) microapp_sdk_log_header_t {
	microapp_sdk_header_t header;
	uint8_t type;
	uint8_t flags;
	uint8_t size;
};

static_assert(sizeof(microapp_sdk_log_header_t) <= MICROAPP_SDK_LOG_HEADER_SIZE);

// Char
struct __attribute__((packed)) microapp_sdk_log_char_t {
	microapp_sdk_log_header_t logHeader;
	uint8_t value;
};

static_assert(sizeof(microapp_sdk_log_char_t) <= MICROAPP_SDK_MAX_PAYLOAD);

// Short
struct __attribute__((packed)) microapp_sdk_log_short_t {
	microapp_sdk_log_header_t logHeader;
	uint16_t value;
};

static_assert(sizeof(microapp_sdk_log_short_t) <= MICROAPP_SDK_MAX_PAYLOAD);

// Uint
struct __attribute__((packed)) microapp_sdk_log_uint_t {
	microapp_sdk_log_header_t logHeader;
	uint32_t value;
};

static_assert(sizeof(microapp_sdk_log_uint_t) <= MICROAPP_SDK_MAX_PAYLOAD);

// Int
struct __attribute__((packed)) microapp_sdk_log_int_t {
	microapp_sdk_log_header_t logHeader;
	int32_t value;
};

static_assert(sizeof(microapp_sdk_log_int_t) <= MICROAPP_SDK_MAX_PAYLOAD);

// Float
struct __attribute__((packed)) microapp_sdk_log_float_t {
	microapp_sdk_log_header_t logHeader;
	float value;
};

static_assert(sizeof(microapp_sdk_log_float_t) <= MICROAPP_SDK_MAX_PAYLOAD);

// Double
struct __attribute__((packed)) microapp_sdk_log_double_t {
	microapp_sdk_log_header_t logHeader;
	double value;
};

static_assert(sizeof(microapp_sdk_log_double_t) <= MICROAPP_SDK_MAX_PAYLOAD);

// String
struct __attribute__((packed)) microapp_sdk_log_string_t {
	microapp_sdk_log_header_t logHeader;
	char str[MICROAPP_SDK_MAX_STRING_LENGTH];
};

static_assert(sizeof(microapp_sdk_log_string_t) <= MICROAPP_SDK_MAX_PAYLOAD);

// Char array
struct __attribute__((packed)) microapp_sdk_log_array_t {
	microapp_sdk_log_header_t logHeader;
	char arr[MICROAPP_SDK_MAX_ARRAY_SIZE];
};

static_assert(sizeof(microapp_sdk_log_array_t) <= MICROAPP_SDK_MAX_PAYLOAD);

/**
 * @struct microapp_sdk_pin_t
 * Struct to control GPIO pins. Pins can be initialized, read or written.
 *
 * @var microapp_sdk_pin_t::header
 * @var microapp_sdk_pin_t::pin         MicroappSdkPin             Specifies the GPIO pin, button or led
 * @var microapp_sdk_pin_t::type        MicroappSdkPinType         Specifies whether to initialize (INIT) or read/write
 * (ACTION)
 * @var microapp_sdk_pin_t::direction   MicroappSdkPinDirection    Specifies whether to set the pin as input or output.
 * Only used with INIT
 * @var microapp_sdk_pin_t::polarity    MicroappSdkPinPolarity     Specifies which pin events to watch. Only used
 * with INIT
 * @var microapp_sdk_pin_t::action      MicroappSdkPinActionType   Specifies whether to read from or write to pin. Only
 * used with ACTION
 * @var microapp_sdk_pin_t::value       MicroappSdkPinValue        Specifies value to write to pin, or field to put read
 * value. Only used with ACTION
 */
struct __attribute__((packed)) microapp_sdk_pin_t {
	microapp_sdk_header_t header;
	uint8_t pin;
	uint8_t type;
	uint8_t direction;
	uint8_t polarity;
	uint8_t action;
	uint8_t value;
};

static_assert(sizeof(microapp_sdk_pin_t) <= MICROAPP_SDK_MAX_PAYLOAD);

/**
 * @struct microapp_sdk_switch_t
 * Struct for switching and dimming the crownstone. Conforms to the general control command protocol
 *
 * @var microapp_sdk_switch_t::header
 * @var microapp_sdk_switch_t::value    MicroappSdkSwitchValue      Specifies what action to take. See enum definition
 * for further info
 */
struct __attribute__((packed)) microapp_sdk_switch_t {
	microapp_sdk_header_t header;
	uint8_t value;
};

static_assert(sizeof(microapp_sdk_switch_t) <= MICROAPP_SDK_MAX_PAYLOAD);

/**
 * @struct microapp_sdk_service_data_t
 * Struct for microapp service data to be advertised by bluenet
 *
 * @var microapp_sdk_service_data_t::header
 * @var microapp_sdk_service_data_t::appUuid       Unique app identifier that will be advertised along with the payload
 * @var microapp_sdk_service_data_t::size          Size of the payload
 * @var microapp_sdk_service_data_t::data          Payload
 */
struct __attribute__((packed)) microapp_sdk_service_data_t {
	microapp_sdk_header_t header;
	uint16_t appUuid;
	uint8_t size;
	uint8_t data[MICROAPP_SDK_MAX_SERVICE_DATA_LENGTH];
};

static_assert(sizeof(microapp_sdk_service_data_t) <= MICROAPP_SDK_MAX_PAYLOAD);

/**
 * @struct microapp_sdk_twi_t
 * Struct for i2c/twi initialization, writes, and reads.
 *
 * @var microapp_sdk_twi_t::header
 * @var microapp_sdk_twi_t::type       MicroappSdkTwiType           Specifies what action to take
 * @var microapp_sdk_twi_t::adress                                  Slave address to write to
 * @var microapp_sdk_twi_t::flags      MicroappSdkTwiFlags          Flags for the command. Currently only includes stop
 * flag
 * @var microapp_sdk_twi_t::size                                    Size of the payload
 * @var microapp_sdk_twi_t::data                                    Payload
 */
struct __attribute__((packed)) microapp_sdk_twi_t {
	microapp_sdk_header_t header;
	uint8_t type;
	uint8_t address;
	uint8_t flags;
	uint8_t size;
	uint8_t buf[MICROAPP_SDK_MAX_TWI_PAYLOAD_SIZE];
};

static_assert(sizeof(microapp_sdk_twi_t) <= MICROAPP_SDK_MAX_PAYLOAD);

/**
 * @struct microapp_sdk_ble_t
 * Struct for Bluetooth Low Energy related messages, excluding mesh. Includes scanning and connecting
 *
 * @var microapp_sdk_ble_t::header
 * @var microapp_sdk_ble_t::type          MicroappSdkBleType       Specifies the type of message. See enum definition
 * for further info
 * @var microapp_sdk_ble_t::address_type                           Type of address
 * @var microapp_sdk_ble_t::address                                Big-endian MAC address. Context depends on type field
 * @var microapp_sdk_ble_t::rssi                                   Received signal strength. For type SCANNED_DEVICE,
 * this is the rssi of the device
 * @var microapp_sdk_ble_t::size                                   Size of payload
 * @var microapp_sdk_ble_t::data                                   Payload. For type SCANNED_DEVICE, this is the
 * advertisement data
 */
struct __attribute__((packed)) microapp_sdk_ble_t {
	microapp_sdk_header_t header;
	uint8_t type;
	uint8_t address_type;
	uint8_t address[MAC_ADDRESS_LENGTH];
	int8_t rssi;
	uint8_t size;
	uint8_t data[MAX_BLE_ADV_DATA_LENGTH];
};

static_assert(sizeof(microapp_sdk_ble_t) <= MICROAPP_SDK_MAX_PAYLOAD);

/**
 * @struct microapp_sdk_mesh_t
 * Struct for mesh message from microapp.
 *
 * @var microapp_sdk_mesh_t::header
 * @var microapp_sdk_mesh_t::type      MicroappSdkMeshType      Specifies the type of message. See enum definition for
 * further info
 * @var microapp_sdk_mesh_t::stoneId                            Indicates the stone id to send to/read from or own stone
 * id. 0 indicates broadcast
 * @var microapp_sdk_mesh_t::size                               Size of payload
 * @var microapp_sdk_mesh_t::data                               Payload.
 */
struct __attribute__((packed)) microapp_sdk_mesh_t {
	microapp_sdk_header_t header;
	uint8_t type;
	uint8_t stoneId;
	uint8_t size;
	uint8_t data[MAX_MICROAPP_MESH_PAYLOAD_SIZE];
};

static_assert(sizeof(microapp_sdk_mesh_t) <= MICROAPP_SDK_MAX_PAYLOAD);

/**
 * @struct microapp_sdk_power_usage_t
 * Struct for microapp power usage requests
 *
 * @var microapp_sdk_power_usage_t::header
 * @var microapp_sdk_power_usage_t::type         MicroappSdkPowerUsageType     Specifies the type of power usage
 * requested. See enum definition
 * @var microapp_sdk_power_usage_t::powerUsage                                 The power usage. Units vary based on type
 */
struct __attribute__((packed)) microapp_sdk_power_usage_t {
	microapp_sdk_header_t header;
	uint8_t type;
	int32_t powerUsage;
};

static_assert(sizeof(microapp_sdk_power_usage_t) <= MICROAPP_SDK_MAX_PAYLOAD);

/**
 * @struct microapp_sdk_presence_t
 * Struct for microapp presence requests
 *
 * @var microapp_sdk_presence_t::header
 * @var microapp_sdk_presence_t::profileId          Specifies the profile for which the presence is requested
 * @var microapp_sdk_presence_t::presenceBitmask    A bitmask where each bit indicates the presence of the profile in a
 * specific location
 */
struct __attribute__((packed)) microapp_sdk_presence_t {
	microapp_sdk_header_t header;
	uint8_t profileId;
	uint64_t presenceBitmask;
};

static_assert(sizeof(microapp_sdk_presence_t) <= MICROAPP_SDK_MAX_PAYLOAD);

/**
 * @struct microapp_sdk_control_command_t
 * Struct with payload conforming to control command protocol for direct handling by command handler
 * See https://github.com/crownstoen/bluenet/blob/master/docs/protocol/PROTOCOL.md#control-packet
 *
 * @var microapp_sdk_control_command_t::header
 * @var microapp_sdk_control_command_t::protocol    The protocol of the command
 * @var microapp_sdk_control_command_t::type        The type of command
 * @var microapp_sdk_control_command_t::size        Size of the payload
 * @var microapp_sdk_control_command_t::payload     Payload
 */
struct __attribute__((packed)) microapp_sdk_control_command_t {
	microapp_sdk_header_t header;
	uint8_t protocol;
	uint16_t type;
	uint16_t size;
	uint8_t payload[MICROAPP_SDK_MAX_CONTROL_COMMAND_PAYLOAD_SIZE];
};

static_assert(sizeof(microapp_sdk_control_command_t) <= MICROAPP_SDK_MAX_PAYLOAD);

/**
 * @struct microapp_sdk_yield_t
 * Struct for microapp yielding to bluenet, e.g. upon completing a setup or loop call, or within an async call (e.g.
 * delay)
 *
 * @var microapp_sdk_yield_t::header
 * @var microapp_sdk_yield_t::type                 MicroappSdkYieldType   The type of yield. See the enum definition.
 * @var microapp_sdk_yield_t::emptyInterruptSlots                         Number of empty slots for interrupts the
 * microapp has. If zero, block new interrupts
 */
struct __attribute__((packed)) microapp_sdk_yield_t {
	microapp_sdk_header_t header;
	uint8_t type;
	uint8_t emptyInterruptSlots;
};

static_assert(sizeof(microapp_sdk_yield_t) <= MICROAPP_SDK_MAX_PAYLOAD);