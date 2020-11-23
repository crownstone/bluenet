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
 * Shortcut for MVP implementation:
 * - We may encounter mac addresses with the same squashed address
 * - Squashing is not invertible
 *
 * Better alternative:
 * - upgrade tracked device tokens to work for any mac? (And they do rotations.. yay)
 */
struct __attribute__((__packed__)) MacAddress {
	static constexpr uint8_t SIZE = 6;
	uint8_t bytes[SIZE] = {0};

	MacAddress() = default;

	MacAddress(const uint8_t * const mac){
		std::memcpy(bytes, mac, SIZE);
	}

	/**
	 * copy constructor, creates deep copy.
	 */
	MacAddress(const MacAddress& other) : MacAddress(other.bytes){
	}

	bool operator==(const MacAddress& other){
		return std::memcmp(bytes,other.bytes,SIZE) == 0;
	}
};

/**
 *
 */
class NearestWitnessReport {
public:
	MacAddress trackable;
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

	NearestWitnessReport(MacAddress mac, int8_t rssi, stone_id_t id) :
			trackable(mac), rssi(rssi), reporter(id) {
	}

	NearestWitnessReport() = default;
};
