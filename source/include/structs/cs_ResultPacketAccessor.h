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
 * Default payload size for a result packet.
 * Requires CS_CHAR_READ_BUF_SIZE to be defined.
 */
static const size_t CS_RESULT_PACKET_DEFAULT_PAYLOAD_SIZE = (g_CS_CHAR_READ_BUF_SIZE - sizeof(result_packet_header_t));

template <cs_buffer_size_t PAYLOAD_SIZE = CS_RESULT_PACKET_DEFAULT_PAYLOAD_SIZE>
class ResultPacketAccessor: BufferAccessor {
public:
	/**
	 * Only set or get fields when this instance is initialized.
	 */
	bool isInitialized() const {
		return _buffer != NULL;
	}

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
	 * Get the result code.
	 */
	cs_ret_code_t getResult() const {
		checkInitialized();
		return _buffer->header.returnCode;
	}

	/**
	 * Set the result code.
	 */
	void setResult(cs_ret_code_t result) {
		checkInitialized();
		_buffer->header.returnCode = result;
	}

	/**
	 * Get the payload.
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
	 * @return                    Size of the payload in bytes.
	 */
	cs_buffer_size_t getPayloadSize() {
		checkInitialized();
		return std::min(_buffer->header.payloadSize, PAYLOAD_SIZE);
	}

	/**
	 * Get the payload buffer.
	 *
	 * @return pointer to the payload buffer.
	 */
	buffer_ptr_t getPayloadBuffer() {
		checkInitialized();
		return _buffer->payload;
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

	cs_ret_code_t setPayloadSize(cs_buffer_size_t size) {
		checkInitialized();
		if (size > getMaxPayloadSize()) {
			return ERR_BUFFER_TOO_SMALL;
		}
		_buffer->header.payloadSize = size;
		return ERR_SUCCESS;
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
		_buffer->header.payloadSize = size;
		memcpy(_buffer->payload, payload, size);
		return ERR_SUCCESS;
	}

	/** @inherit */
	cs_ret_code_t assign(buffer_ptr_t buffer, cs_buffer_size_t size) {
		assert(sizeof(result_packet_header_t) + PAYLOAD_SIZE <= size, STR_ERR_BUFFER_NOT_LARGE_ENOUGH);
		LOGd(FMT_ASSIGN_BUFFER_LEN, buffer, size);
		_buffer = (result_packet_t<PAYLOAD_SIZE>*)buffer;
		return ERR_SUCCESS;
	}

	/** @inherit */
	cs_buffer_size_t getSerializedSize() const {
		checkInitialized();
		return sizeof(result_packet_header_t) + _buffer->header.payloadSize;
	}

	/** @inherit */
	cs_buffer_size_t getBufferSize() const {
		checkInitialized();
		return sizeof(result_packet_header_t) + PAYLOAD_SIZE;
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
	result_packet_t<PAYLOAD_SIZE>* _buffer;

	void checkInitialized() const {
		assert(isInitialized(), "Buffer not initialized");
	}

private:
};
