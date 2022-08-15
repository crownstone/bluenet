/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 9, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <common/cs_Types.h>

#include <cstdint>

/**
 * An event which can be sent over the event bus. The event bus is synchronous.
 *
 * There is no possibility to send a mere uint8_t or other primitive data type. Everything is passed by pointer. Hence,
 * first create a local variable on the stack, then point to it.
 *
 *    uint8_t data = 1;
 *    event_t event(CS_TYPE::EVT_XXX, &data, sizeof(data));
 *
 * There is no memory allocation or deallocation happening in this class. The caller is responsible for this. If you
 * use the stack, make sure you dispatch the event when the data on the stack is still valid.
 */
class event_t {
public:
	event_t(CS_TYPE type, void* data, size16_t size, const cmd_source_with_counter_t& source, const cs_result_t& result)
			: type(type), data(data), size(size), source(source), result(result) {}

	event_t(CS_TYPE type, void* data, size16_t size, const cmd_source_with_counter_t& source)
			: type(type), data(data), size(size), source(source), result() {}

	event_t(CS_TYPE type, void* data, size16_t size, const cs_result_t& result)
			: type(type), data(data), size(size), source(), result(result) {}

	event_t(CS_TYPE type, void* data, size16_t size) : type(type), data(data), size(size), source(), result() {}

	event_t(CS_TYPE type) : event_t(type, nullptr, 0) {}

	// Type of event.
	CS_TYPE type;

	// Event data.
	void* data;

	// Size of the event data.
	size16_t size;

	/**
	 * Source of the command (optional).
	 */
	cmd_source_with_counter_t source;

	/**
	 * Result of the event (optional).
	 *
	 * Sender of the event allocates the a buffer and assigns it to result.buf if it accepts response data.
	 * Handler of the event adjusts the result code, writes to result.buf.data and sets result.dataSize accordingly.
	 */
	cs_result_t result;

	/**
	 * Utility function to get the data as uint8_t*.
	 */
	inline uint8_t* getData() { return static_cast<uint8_t*>(data); }

	/**
	 * Utility function so that not every file needs to include the eventdispatcher.
	 *
	 * This runs the handleEvent(...) method of all registered listeners.
	 */
	void dispatch();
};
