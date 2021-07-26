/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 20, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cstdint> // for uint8_t
#include <cstring> // for memcmp, memcpy
#include <logging/cs_Logger.h>
#include <protocol/mesh/cs_MeshModelPackets.h>

/**
 * Mesh representation of a trackable device.
 *
 * Firmware may assume that a TrackableId uniquely identifies
 * a physical device to be tracked.
 *
 * Shortcut for MVP implementation: just copy 6 byte mac.
 * Improve when TrackableParser is growing.
 */
struct __attribute__((__packed__)) TrackableId {
	static constexpr uint8_t SIZE = 6;
	uint8_t bytes[SIZE] = {0};

	TrackableId() = default;

	// REVIEW: Missing size (or use mac address struct)
	TrackableId(const uint8_t * const mac){
		std::memcpy(bytes, mac, SIZE);
	}

	bool operator==(const TrackableId& other) const {
		return std::memcmp(bytes,other.bytes,SIZE) == 0;
	}

	/**
	 * Enables to use uuids as keys for maps/sets etc.
	 */
	bool operator<(const TrackableId& other) const {
		return std::memcmp(bytes,other.bytes,SIZE) < 0;
	}

	// REVIEW: string as argument can't be left out from binary size.
	void print(const char* headerstr) {
		LOGnone("%s mac=[%2x,%2x,%2x,%2x,%2x,%2x]",
				headerstr,
				bytes[0],
				bytes[1],
				bytes[2],
				bytes[3],
				bytes[4],
				bytes[5]);
	}
};
