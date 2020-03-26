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

	cs_result_t result;

    /**
     * Utility function so that not every file needs to include the eventdispatcher.
     */
    void dispatch();
};
