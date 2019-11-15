/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 9, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <common/cs_Types.h>

#include <cstdint>

struct event_result_t {
	/**
	 * Return code.
	 *
	 * Should be set by the class that handles the event.
	 */
	cs_ret_code_t returnCode = ERR_EVENT_UNHANDLED;

	/**
	 * Buffer to put the result data in.
	 *
	 * Can be NULL.
	 */
	cs_data_t buf;

	/**
	 * Length of the data in the buffer.
	 *
	 * Should be set by the class that handles the event.
	 */
	cs_buffer_size_t dataSize = 0;
};

class event_t {
    public:
	event_t(CS_TYPE type, void * data, size16_t size) :
		type(type),
		data(data),
		size(size),
		result()
	{}

	event_t(CS_TYPE type) :
		event_t(type, nullptr, 0)
	{}

	CS_TYPE type;

	void *data;
	inline uint8_t* getData(){ return static_cast<uint8_t*>(data); }

	size16_t size;

	event_result_t result;

    /**
     * Utility function so that not every file needs to include the eventdispatcher.
     */
    void dispatch();
};
