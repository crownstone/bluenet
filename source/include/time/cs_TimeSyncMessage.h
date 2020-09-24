/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 6, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdint>

/**
 * message to be sent over mesh
 */
struct __attribute__((__packed__)) high_resolution_time_stamp_t {
	uint32_t posix_s;  // seconds since epoch
	uint16_t posix_ms;  // miliseconds passed since posix_s.
};

/**
 * event sent over the internal event bus upon reception of new mesh
 * time stamp msg.
 */
struct __attribute__((__packed__)) time_sync_message_t {
	high_resolution_time_stamp_t stamp;
	stone_id_t root_id;
	// (could squeeze max 64 hops into the miliseconds field of .stamp
	// because that won't need values above 1000.)
};
