/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 17, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <protocol/cs_ErrorCodes.h>
#include <protocol/cs_Packets.h>

/**
 * Packets (structs) that are used internally in the firmware, and can be changed freely.
 * This means packets that:
 * - do not go over the air
 * - do not go over uart
 * - are not stored in flash
 *
 * Constructors can be added, as they do not impact the size of the struct.
 *
 * If the definition becomes large, move it to its own file and include it in this file.
 */

/**
 * Variable length data encapsulation in terms of length and pointer to data.
 */
struct cs_data_t {
	buffer_ptr_t data    = nullptr; /** < Pointer to data. */
	cs_buffer_size_t len = 0;       /** < Length of data. */

	cs_data_t() : data(nullptr), len(0) {}
	cs_data_t(buffer_ptr_t buf, cs_buffer_size_t size) : data(buf), len(size) {}
};

/**
 * Variable length data encapsulation in terms of length and pointer to data.
 */
struct cs_const_data_t {
	const uint8_t* data = nullptr;      /** < Pointer to data. */
	cs_buffer_size_t len = 0;      /** < Length of data. */

	cs_const_data_t():
		data(nullptr),
		len(0)
	{}
	cs_const_data_t(const uint8_t* buf, cs_buffer_size_t size):
		data(buf),
		len(size)
	{}
};

struct cs_result_t {
	/**
	 * Return code.
	 *
	 * Should be set by the handler.
	 */
	cs_ret_code_t returnCode = ERR_EVENT_UNHANDLED;

	/**
	 * Buffer to put the result data in (can be nullptr).
	 */
	cs_data_t buf;

	/**
	 * Length of the data in the buffer.
	 *
	 * Should be set by the handler.
	 */
	cs_buffer_size_t dataSize = 0;

	cs_result_t() :                         buf() {}
	cs_result_t(cs_data_t buf) :            buf(buf) {}
	cs_result_t(cs_ret_code_t returnCode) : returnCode(returnCode), buf() {}
};

/**
 * Copy of BLE_GAP_ADDR_TYPES.
 *
 * More details, see:
 * https://devzone.nordicsemi.com/f/nordic-q-a/27012/how-to-distinguish-between-random-and-public-gap-addresses
 * https://devzone.nordicsemi.com/f/nordic-q-a/2084/gap-address-types
 */
enum CS_ADDRESS_TYPE {
	CS_ADDRESS_TYPE_PUBLIC                        = 0,    // Public (registered) static address.
	CS_ADDRESS_TYPE_RANDOM_STATIC                 = 1,    // Random static address (can only change at boot).
	CS_ADDRESS_TYPE_RANDOM_PRIVATE_RESOLVABLE     = 2,    // Random resolvable address (can change at any moment).
	CS_ADDRESS_TYPE_RANDOM_PRIVATE_NON_RESOLVABLE = 3,    // Random address (can change at any moment).
	CS_ADDRESS_TYPE_ANONYMOUS                     = 0x7F,  // No address is advertised.
};

struct __attribute__((packed)) device_address_t {
	uint8_t address[MAC_ADDRESS_LEN];
	uint8_t addressType = CS_ADDRESS_TYPE_RANDOM_STATIC;
};

/**
 * Scanned device.
 */
struct __attribute__((packed)) scanned_device_t {
	int8_t rssi;
	uint8_t address[MAC_ADDRESS_LEN];
	bool resolvedPrivateAddress;
	uint8_t addressType;  // See CS_ADDRESS_TYPE
	uint8_t channel;
	uint8_t dataSize;
	uint8_t* data;  // What is the content of this field?
					// See ble_gap_evt_adv_report_t
	// More possibilities: addressType, connectable, isScanResponse, directed, scannable, extended advertisements, etc.
};

/**
 * A single multi switch command.
 * switchCmd: 0 = off, 100 = fully on.
 */
struct __attribute__((packed)) internal_multi_switch_item_cmd_t {
	uint8_t switchCmd;
};

/**
 * A single multi switch packet, with target id.
 */
struct __attribute__((packed)) internal_multi_switch_item_t {
	stone_id_t id;
	internal_multi_switch_item_cmd_t cmd;
};

inline bool cs_multi_switch_item_is_valid(internal_multi_switch_item_t* item, size16_t size) {
	return (size == sizeof(internal_multi_switch_item_t) && item->id != 0);
}

struct __attribute__((packed)) control_command_packet_t {
	uint8_t protocolVersion = CS_CONNECTION_PROTOCOL_VERSION;
	CommandHandlerTypes type;
	buffer_ptr_t data;
	size16_t size;
	EncryptionAccessLevel accessLevel;
};

/**
 * Mesh control command packet.
 */
struct __attribute__((__packed__)) mesh_control_command_packet_t {
	mesh_control_command_packet_header_t header;
	stone_id_t* targetIds;
	control_command_packet_t controlCommand;
};

/**
 * How reliable a mesh message should be.
 *
 * For now, the associated number is the number of times the message gets sent.
 */
enum cs_mesh_msg_reliability {
	CS_MESH_RELIABILITY_INVALID = 0,
	CS_MESH_RELIABILITY_LOWEST  = 1,
	CS_MESH_RELIABILITY_LOW     = 3,
	CS_MESH_RELIABILITY_MEDIUM  = 5,
	CS_MESH_RELIABILITY_HIGH    = 10,
};

/**
 * How urgent a message is.
 *
 * Lower urgency means that it's okay if the message is not sent immediately.
 */
enum cs_mesh_msg_urgency {
	CS_MESH_URGENCY_LOW,
	CS_MESH_URGENCY_HIGH,
};

/**
 * Struct to communicate a mesh message.
 * type            Type of message.
 * flags           How to send the message.
 * reliability     Timeout for acked messages, number of transmission for unacked messages. Use 0 for the default value.
 * urgency         How quick the message should be sent.
 * idCount         Number of IDs, for targeted messages.
 * targetIds       Pointer to array with targeted stone IDs.
 * size            Size of the payload.
 * payload         The payload.
 */
struct cs_mesh_msg_t {
	cs_mesh_model_msg_type_t type;
	mesh_control_command_packet_flags_t flags;
	uint8_t reliability         = 0;
	cs_mesh_msg_urgency urgency = CS_MESH_URGENCY_LOW;
	uint8_t idCount             = 0;
	stone_id_t* targetIds       = nullptr;
	size16_t size               = 0;
	uint8_t* payload            = nullptr;
};

/**
 * Struct to communicate received state of other stones.
 * rssi            RSSI to this stone, or 0 if not received the state directly from that stone.
 * serviceData     State of the stone.
 */
struct state_external_stone_t {
	service_data_encrypted_t data;
};

/**
 * Unparsed background advertisement.
 */
struct __attribute__((__packed__)) adv_background_t {
	uint8_t protocol;
	uint8_t sphereId;
	uint8_t* data;
	uint8_t dataSize;
	uint8_t* macAddress;
	int8_t rssi;
};

/**
 * Parsed background advertisement.
 */
struct __attribute__((__packed__)) adv_background_parsed_t {
	uint8_t* macAddress;
	int8_t adjustedRssi;
	uint8_t locationId;
	uint8_t profileId;
	uint8_t flags;
};

struct __attribute__((__packed__)) adv_background_parsed_v1_t {
	uint8_t* macAddress;
	int8_t rssi;
	uint8_t deviceToken[TRACKED_DEVICE_TOKEN_SIZE];
};

struct profile_location_t {
	uint8_t profileId;
	uint8_t locationId;
	bool fromMesh  = false;
	bool simulated = false;
};

struct __attribute__((packed)) internal_register_tracked_device_packet_t {
	register_tracked_device_packet_t data;
	uint8_t accessLevel = NOT_SET;
};

typedef internal_register_tracked_device_packet_t internal_update_tracked_device_packet_t;

struct __attribute__((packed)) internal_tracked_device_heartbeat_packet_t {
	tracked_device_heartbeat_packet_t data;
	uint8_t accessLevel = NOT_SET;
};

#define CS_ADC_REF_PIN_NOT_AVAILABLE 255
#define CS_ADC_PIN_VDD 100

/**
 * Struct to configure an ADC channel.
 *
 * pin:                 The AIN pin to sample.
 * rangeMilliVolt:      The range in mV of the pin.
 * referencePin:        The AIN pin to be subtracted from the measured voltage.
 */
struct __attribute__((packed)) adc_channel_config_t {
	adc_pin_id_t pin;
	uint32_t rangeMilliVolt;
	adc_pin_id_t referencePin;
};

/**
 * Struct to configure the ADC.
 *
 * channelCount:        The amount of channels to sample.
 * channels:            The channel configs.
 * samplingIntervalUs:  The sampling interval in μs (each interval, all channels are sampled once).
 */
struct __attribute__((packed)) adc_config_t {
	adc_channel_id_t channelCount;
	adc_channel_config_t channels[CS_ADC_NUM_CHANNELS];
	uint32_t samplingIntervalUs;
};

/**
 * Result struct after configuring an ADC channel. Has all the info to put the sample values in context.
 *
 * Usage: Vpin = sampleVal / maxSampleValue * maxValueMilliVolt * 1000 + Vpinref
 *
 * pin:                 The AIN pin of this channel.
 * referencePin:        The AIN pin that was subtracted from the measured voltage on <pin>.
 * samplingIntervalUs:  The sampling interval in μs.
 * minValueMilliVolt:   Minimum of the measured voltage (Vpin - Vpinref) range, in mV.
 * maxValueMilliVolt:   Maximum of the measured voltage (Vpin - Vpinref) range, in mV.
 * minSampleValue:      Minimum of the sample value range.
 * maxSampleValue:      Maximum of the sample value range.
 */
struct __attribute__((packed)) adc_channel_config_result_t {
	adc_pin_id_t pin;
	adc_pin_id_t referencePin;
	uint16_t samplingIntervalUs;
	int16_t minValueMilliVolt;
	int16_t maxValueMilliVolt;
	adc_sample_value_t minSampleValue;
	adc_sample_value_t maxSampleValue;
};

/**
 * Struct communicated from the ADC class when it's done sampling a buffer.
 */
struct adc_buffer_t {
	/**
	 * Whether this buffer has valid data.
	 *
	 * This may change at any moment, so make sure to check it after calculations and be ready to revert.
	 */
	bool valid = false;

	/**
	 * Sequence number.
	 *
	 * Increased by 1 for each consecutive buffer. Can be used to check if you missed a buffer.
	 */
	adc_buffer_seq_nr_t seqNr = 0;

	/**
	 * The ADC config that was used to sample this buffer.
	 */
	adc_channel_config_result_t config[CS_ADC_NUM_CHANNELS];

	/**
	 * Pointer to the samples.
	 */
	adc_sample_value_t* samples = nullptr;
};

struct hub_data_reply_t {
	cs_ret_code_t retCode;
	cs_data_t data;
};

struct microapp_advertise_request_t {
	uint8_t type;  // 0 for encrypted service data.
	uint8_t version;
	uint16_t appUuid;
	cs_data_t data;
};

struct microapp_upload_internal_t {
	microapp_upload_t header;
	cs_data_t data;
};

struct mesh_topo_mac_result_t {
	uint8_t stoneId;
	uint8_t macAddress[MAC_ADDRESS_LEN];
};
