/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 9, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <common/cs_Types.h>

#include <cstdint>

class event_t {
public:
	event_t(CS_TYPE type, void * data, size16_t size, const cmd_source_with_counter_t& source, const cs_result_t& result) :
		type(type),
		data(data),
		size(size),
		source(source),
		result(result)
	{}

	event_t(CS_TYPE type, void * data, size16_t size, const cmd_source_with_counter_t& source) :
		type(type),
		data(data),
		size(size),
		source(source),
		result()
	{}

	event_t(CS_TYPE type, void * data, size16_t size, const cs_result_t& result) :
		type(type),
		data(data),
		size(size),
		source(),
		result(result)
	{}

	event_t(CS_TYPE type, void * data, size16_t size) :
		type(type),
		data(data),
		size(size),
		source(),
		result()
	{}

	/**
	 * Expands a locally defined array to its pointer and length.
	 *
	 * Usage:
	 * uint8_t result[10];
	 * event_t event(CS_TYPE::FOO, result);
	 * event.dispatch();
	 *
	 */
	template<int N>
	inline event_t(CS_TYPE type, uint8_t data[N]) :
		event_t(type, data, N)
	{}

	event_t(CS_TYPE type) :
		event_t(type, nullptr, 0)
	{}

	// Type of event.
	CS_TYPE type;

	// Event data.
	void *data;

	// Size of the event data.
	size16_t size;

	/**
	 * Source of the command (optional).
	 */
	cmd_source_with_counter_t source;

	/**
	 * Result of the event (optional).
	 * The buffer is set by the sender of the event.
	 * The result code and data size is set by the handler of the event.
	 */
	cs_result_t result;

	/**
	 * Utility function to get the data as uint8_t*.
	 */
	inline uint8_t* getData() {
		return static_cast<uint8_t*>(data);
	}

	/**
	 * Utility function so that not every file needs to include the eventdispatcher.
	 *
	 * This runs the handleEvent(...) method of all registered listeners.
	 */
	void dispatch();
};
