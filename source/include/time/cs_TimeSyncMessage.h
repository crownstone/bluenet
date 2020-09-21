/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 6, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdint>


struct __attribute__((__packed__)) high_resolution_time_stamp_t {
	uint32_t posix_s;  // seconds since epoch
	uint16_t posix_ms;  // miliseconds passed since posix_s.
};

/**
 *
 */
struct __attribute__((__packed__)) time_sync_message_t {
	high_resolution_time_stamp_t stamp;
	stone_id_t root_id;
};
