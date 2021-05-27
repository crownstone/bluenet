/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 27, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <stddef.h>
#include <cstdint>

typedef const uint8_t* filter_entry_t;

/**
 * Data content of the exact match filter.
 */
struct __attribute__((__packed__)) exact_match_filter_data_t {
	uint16_t itemCount;
	uint16_t itemSize;
	uint8_t itemArray[];
};
