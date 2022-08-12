/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 3, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <structs/cs_BufferAccessor.h>

/**
 * A wrapper around a raw uint8_t array (buffer) to easily read, write from it.
 *
 * Example usage:
 *
 * struct some_packet_t{
 *	float val;
 *	uint32_t prescale;
 *	uint16_t id;
 *	uint16_t unused;
 * };
 *
 * some_packet_t pack;
 * pack.val = 1234.0f;
 * pack.prescale = 100;
 * pack.id = 0x4321;
 * pack.unused = 0xabcd;
 *
 * uint8_t buff[16] = {0};
 * StreamBufferAccessor stream;
 * stream.assign(buff,16);
 *
 * stream.put(pack); // (or put individual values)
 * stream.reset(); // (or use a stream obtained from a bluetooth buffer)
 *
 * some_packet_t* received = nullptr;
 *
 * stream.get(received);
 *
 * if(received != nullptr){
 * 	// yay, use your data
 * }
 *
 *
 * Note:
 * Classes/Structs that need to be serialized, either need to be memcopy safe,
 * or they need to specialize the get/put methods in order to handle serialization logic.
 * These classes also need to specialize the size_check<T>
 */
class StreamBufferAccessor {
public:
	StreamBufferAccessor(buffer_ptr_t buffer, cs_buffer_size_t size) { assign(buffer, size); }
	StreamBufferAccessor(cs_data_t data) : StreamBufferAccessor(data.data, data.len) {}

	/**
	 * Resets the stream buffer accessor to construction state,
	 * such that the next operation will read/write will happen from
	 * the first byte of the buffer.
	 */
	void reset() {
		internal_status = ERR_SUCCESS;
		buff_curr       = buff_begin;
	}

	/**
	 * Binds this stream buffer accessor to the given buffer pointer and
	 * tell it that there are size bytes available.
	 */
	/** @inherit */
	cs_ret_code_t assign(buffer_ptr_t buffer, cs_buffer_size_t size) {
		buff_begin = buffer;
		buff_end   = buffer + size;
		reset();

		return ERR_SUCCESS;
	}

	/** @inherit */
	cs_buffer_size_t getSerializedSize() const { return buff_curr - buff_begin; }

	/** @inherit */
	cs_buffer_size_t getBufferSize() const { return buff_end - buff_begin; }

	/** @inherit */
	cs_buffer_size_t getRemainingCapacity() const { return buff_end - buff_curr; }

	/** @inherit */
	cs_data_t getSerializedBuffer() { return cs_data_t(buff_begin, getBufferSize()); }

	// ============ Get ============

	/**
	 * If the stream buffer is in state OK, and remaining packet size
	 * is available, assign the packet_ptr to the current read address
	 * and increment it.
	 *
	 * This will not make any copies.
	 */
	template <class T>
	StreamBufferAccessor& get(T*& packet_ptr) {
		if (!status_check()) {
			// failed status check
			// ignore get request when failure
			return *this;
		}

		if (!size_check<T>()) {
			// failed size check
			packet_ptr      = nullptr;
			internal_status = ERR_NO_SPACE;
		}
		else {
			// write to the out-parameter packet_ptr
			packet_ptr = reinterpret_cast<T*>(buff_curr);
			increment_buff<T>();
		}

		return *this;
	}

	/**
	 * Same as the pointer version of get, but this will copy its value
	 * into the lvalue its return value is to bind to and will try to construct
	 * a default value if the stream is in error state or doesn't have enough
	 * bytes left.
	 */
	template <class T>
	T get() {
		if (!size_check<T>() && !status_check()) {
			increment_buff<T>();
			return *reinterpret_cast<T*>(buff_curr);
		}

		// try to return a default when possible
		internal_status = ERR_NO_SPACE;
		return T();
	}

	// ============ Set ============

	/**
	 * Put packet into the stream, if possible. Then increment the current
	 * buffer pointer.
	 */
	template <class T>
	void put(T packet) {
		if (size_check<T>() && status_check()) {
			// copy into the buffer, after that, increment
			*reinterpret_cast<T*>(buff_curr) = packet;
			increment_buff<T>();
		}
		else {
			// failed size_check
			internal_status = ERR_NO_SPACE;
		}
	}

protected:
	// returns false on failure
	template <class T>
	inline bool size_check() {
		return buff_curr + sizeof(T) <= buff_end;
	}

	template <class T>
	inline void increment_buff() {
		buff_curr += sizeof(T);
	}

	// returns false on failure
	inline bool status_check() { return internal_status == ERR_SUCCESS; }

private:
	buffer_ptr_t buff_begin;
	buffer_ptr_t buff_curr;
	buffer_ptr_t buff_end;

	cs_ret_code_t internal_status;
};
