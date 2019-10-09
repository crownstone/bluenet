/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 9, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <common/cs_Types.h>

struct event_t {
	event_t(CS_TYPE type, void * data, size16_t size): type(type), data(data), size(size) {
	}

	event_t(CS_TYPE type): type(type), data(NULL), size(0) {
	}

	CS_TYPE type;

	void *data;

	size16_t size;
};