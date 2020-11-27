/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 20, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <protocol/mesh/cs_MeshModelPackets.h>

#include <cstdint> // for uint8_t
#include <cstring> // for memcmp, memcpy

/**
 * Mesh representation of a trackable device.
 *
 * Shortcut for MVP implementation: just copy 6 byte mac.
 *
 * Better alternative:
 * - register tracked devices in a whitelist of some sorts? (And they do rotations.. yay)
 */
struct __attribute__((__packed__)) TrackableId {
	static constexpr uint8_t SIZE = 6;
	uint8_t bytes[SIZE] = {0};

	TrackableId() = default;

	TrackableId(const uint8_t * const mac){
		std::memcpy(bytes, mac, SIZE);
	}

	/**
	 * copy constructor, creates deep copy.
	 */
	TrackableId(const TrackableId& other) : TrackableId(other.bytes){
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
};

/**
 *
 */
class NearestWitnessReport {
public:
	TrackableId trackable;
	int8_t rssi = 0;
	stone_id_t reporter = 0;

	/**
	 * copy constructor enables assignment.
	 */
	NearestWitnessReport(NearestWitnessReport &other) :
			trackable(other.trackable),
			rssi(other.rssi),
			reporter(other.reporter) {
	}

	NearestWitnessReport(TrackableId mac, int8_t rssi, stone_id_t id) :
			trackable(mac), rssi(rssi), reporter(id) {
	}

	NearestWitnessReport() = default;
};
