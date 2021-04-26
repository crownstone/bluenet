/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 23, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdint>
#include <cfg/cs_StaticConfig.h>
#include <logging/cs_Logger.h>
#include <protocol/cs_ErrorCodes.h>
#include <structs/cs_BufferAccessor.h>
#include <structs/cs_PacketsInternal.h>
#include <util/cs_Error.h>

/**
 * Default payload size for a control packet.
 * Requires CS_CHAR_WRITE_BUF_SIZE to be defined.
 */
//#define CS_CONTROL_PACKET_DEFAULT_PAYLOAD_SIZE (CS_CHAR_WRITE_BUF_SIZE - sizeof(control_packet_header_t))
static constexpr size_t CS_CONTROL_PACKET_DEFAULT_PAYLOAD_SIZE = (g_CS_CHAR_WRITE_BUF_SIZE - sizeof(control_packet_header_t));

template <cs_buffer_size_t PAYLOAD_SIZE = CS_CONTROL_PACKET_DEFAULT_PAYLOAD_SIZE>
class ControlPacketAccessor: BufferAccessor {
public:
	/**
	 * Get the protocol version.
	 */
	uint8_t getProtocolVersion() const {
		checkInitialized();
		return _buffer->header.protocolVersion;
	}

	/**
	 * Set the protocol version.
	 */
	void setProtocolVersion(uint8_t protocol) {
		checkInitialized();
		_buffer->header.protocolVersion = protocol;
	}

	/**
	 * Get the command type.
	 */
	cs_control_cmd_t getType() const {
		checkInitialized();
		return _buffer->header.commandType;
	}

	/**
	 * Set the command type.
	 */
	void setType(cs_control_cmd_t type) {
		checkInitialized();
		_buffer->header.commandType = type;
	}

	/**
	 * Get pointer to the payload.
	 *
	 * @return                    Data struct with pointer to payload and size of the payload in bytes.
	 */
	cs_data_t getPayload() {
		checkInitialized();
		cs_data_t data;
		data.data = _buffer->payload;
		data.len = getPayloadSize();
		return data;
	}

	/**
	 * Get the size of the payload.
	 *
	 * Will never be larger than what fits in the buffer.
	 *
	 * @return                    Size of the payload.
	 */
	cs_buffer_size_t getPayloadSize() {
		checkInitialized();
		return std::min(_buffer->header.payloadSize, PAYLOAD_SIZE);
	}

	/**
	 * Get the capacity of the payload.
	 *
	 * @return                    Max size of the payload in bytes.
	 */
	cs_buffer_size_t getMaxPayloadSize() {
		checkInitialized();
		return PAYLOAD_SIZE;
	}

	/**
	 * Copy payload into the packet.
	 *
	 * @param[in] payload         Pointer to the payload.
	 * @param[in] size            Size of the payload.
	 * @return error code.
	 */
	cs_ret_code_t setPayload(buffer_ptr_t payload, cs_buffer_size_t size) {
		checkInitialized();
		if (size > PAYLOAD_SIZE) {
			LOGe(STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
			return ERR_BUFFER_TOO_SMALL;
		}
		if (size != 0 && payload == nullptr) {
			LOGe("Null pointer");
			return ERR_BUFFER_UNASSIGNED;
		}
		_buffer->header.payloadSize = size;
		if (size && _buffer->payload != payload) {
			memmove(_buffer->payload, payload, size);
		}
		else {
			LOGd("Skip copy");
		}
		return ERR_SUCCESS;
	}

	/** @inherit */
	cs_ret_code_t assign(buffer_ptr_t buffer, cs_buffer_size_t size) {
		assert(sizeof(control_packet_header_t) + PAYLOAD_SIZE <= size, STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
		LOGd(FMT_ASSIGN_BUFFER_LEN, buffer, size);
		_buffer = (control_packet_t<PAYLOAD_SIZE>*)buffer;
		return ERR_SUCCESS;
	}

	/** @inherit */
	cs_buffer_size_t getSerializedSize() const {
		checkInitialized();
		return sizeof(control_packet_header_t) + _buffer->header.payloadSize;
	}

	/** @inherit */
	cs_buffer_size_t getBufferSize() const {
		checkInitialized();
		return sizeof(control_packet_header_t) + PAYLOAD_SIZE;
	}

	/** @inherit */
	cs_data_t getSerializedBuffer() {
		checkInitialized();
		cs_data_t data;
		data.data = (buffer_ptr_t)_buffer;
		data.len = getSerializedSize();
		return data;
	}

protected:
	/**
	 * Pointer to the packet to be sent.
	 */
	control_packet_t<PAYLOAD_SIZE>* _buffer;

	void checkInitialized() const {
		assert(_buffer != NULL, "Butter not initialized");
	}

private:
};
