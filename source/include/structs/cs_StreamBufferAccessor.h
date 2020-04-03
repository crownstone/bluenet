/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 3, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <structs/cs_BufferAccessor.h>

class ResultPacketAccessor: BufferAccessor {
public:
	/** @inherit */
	cs_ret_code_t assign(buffer_ptr_t buffer, cs_buffer_size_t size) {
		_buffer.data = buffer;
		_buffer.len = size;
		dataSize = 0;
		return ERR_SUCCESS;
	}

	/** @inherit */
	cs_buffer_size_t getSerializedSize() const {
		return dataSize;
	}

	/** @inherit */
	cs_buffer_size_t getBufferSize() const {
		return _buffer.len;
	}

	/** @inherit */
	cs_data_t getSerializedBuffer() {
		return cs_data_t(_buffer.data, dataSize);
	}

	/**
	 * Put a value in the buffer at the current position, and increment the position.
	 * Value will be copied to the buffer.
	 *
	 * @param[in] val             Value to put in the buffer.
	 * @return                    ERR_SUCCESS on success.
	 */
	cs_ret_code_t put(uint8_t val);

	/**
	 * Get a value from the buffer at the current position, and increment the position.
	 * Value will be copied from the buffer.
	 *
	 * @param[out] val            Will be set to the value from the buffer.
	 * @return                    ERR_SUCCESS on success.
	 */
	cs_ret_code_t get(uint8_t & val);

	/**
	 * Put a packet in the buffer at the current position, and increment the position.
	 * Data is not copied, but you get a pointer to the buffer to fill.
	 *
	 * @param[out] packet         Will be set to point at the buffer.
	 * @param[in] packetSize      Size of the packet.
	 * @return                    ERR_SUCCESS on success.
	 */
	cs_ret_code_t put(void* packet, cs_buffer_size_t packetSize);

	/**
	 * Get a packet from the buffer at the current position, and increment the position.
	 * Data is not copied, but you get a pointer to the buffer.
	 *
	 * @param[out] packet         Will be set to point at the buffer.
	 * @param[in] packetSize      Size of the packet.
	 * @return                    ERR_SUCCESS on success.
	 */
	cs_ret_code_t get(void* packet, cs_buffer_size_t packetSize);

private:
	cs_data_t _buffer = cs_data_t();
	cs_buffer_size_t dataSize = 0;
};
