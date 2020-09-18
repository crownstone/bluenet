/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 6, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdint>


struct __attribute__((__packed__)) high_resolution_time_stamp_t {
	uint32_t posix_time;

	// fractional part is represented in rtc ticks.
	uint32_t rtc_ticks;
}

/**
 *
 */
struct __attribute__((__packed__)) time_sync_message_t {
	high_resolution_time_stamp_t stamp;
	stone_id_t id;
};
