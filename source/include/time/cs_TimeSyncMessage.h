/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 6, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <protocol/cs_Typedefs.h>

#include <cstdint>

/**
 * Versioned timestamp with milliseconds precision.
 */
struct __attribute__((__packed__)) high_resolution_time_stamp_t {
	uint32_t posix_s  = 0;  // seconds since epoch
	uint16_t posix_ms = 0;  // milliseconds passed since posix_s.
	uint8_t version   = 0;  // synchronization version, when 0, the timestamp is not posix based.
};

/**
 * event sent over the internal event bus upon reception of new mesh
 * time stamp msg.
 */
struct __attribute__((__packed__)) time_sync_message_t {
	high_resolution_time_stamp_t stamp;
	stone_id_t srcId;  // The stone ID of this time. Set to 0 to force using this timestamp.
};
