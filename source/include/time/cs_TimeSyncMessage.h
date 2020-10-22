/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 6, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdint>
#include <protocol/cs_Typedefs.h>

/**
 * Versioned timestamp with milliseconds precision.
 */
struct __attribute__((__packed__)) high_resolution_time_stamp_t {
	uint32_t posix_s;        // seconds since epoch
	uint16_t posix_ms;       // milliseconds passed since posix_s.
	uint8_t  version;        // synchronization version
};

/**
 * event sent over the internal event bus upon reception of new mesh
 * time stamp msg.
 */
struct __attribute__((__packed__)) time_sync_message_t {
	high_resolution_time_stamp_t stamp;
	stone_id_t root_id; // @arend isn't this just the source Id ? I got confused into thinking this was what the sending node believes to be the root id.
};
