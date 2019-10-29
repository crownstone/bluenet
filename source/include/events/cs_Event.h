/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 9, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <common/cs_Types.h>

class event_t {
    public:
	event_t(CS_TYPE type, void * data, size16_t size) :
		type(type),
		data(data),
		size(size),
		returnCode(ERR_EVENT_UNHANDLED) 
	{}

	event_t(CS_TYPE type) : 
		event_t(type, NULL, 0)
	{}

	CS_TYPE type;

	void *data;

	size16_t size;

	/**
	 * Will be set to 
	 */
	cs_ret_code_t returnCode;
	/**
	 * A field that can be used by handlers to return a value to
	 * the dispatching scope.
	 */
	cs_data_t result = {};

    /**
     * Validates [data] for nullptr and [size] to match expected size of [type].
     */
    void dispatch();
};