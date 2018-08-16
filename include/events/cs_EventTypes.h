/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 6, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <stdint.h>
#include <protocol/cs_StateTypes.h>

/**
 * Event types.
 *
 * TODO: This are not all event types... These are also Config values, State values, etc.
 */

struct event_t {
	event_t(CS_TYPE type, void * data, size16_t size): type(type), data(data), size(size) { }
	event_t(CS_TYPE type): type(type), data(NULL), size(0) { }
	CS_TYPE type;	
	void *data;
	size16_t size;
};

struct __attribute__((packed)) evt_do_reset_delayed_t {
	uint8_t resetCode;
	uint16_t delayMs;
};
