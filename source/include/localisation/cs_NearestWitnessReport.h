/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 20, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

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
struct __attribute__((__packed__)) SquashedMacAddress {
	uint8_t bytes[3];

	SquashedMacAddress() = default;

	/**
	 * copy constructor.
	 */
	SquashedMacAddress(const SquashedMacAddress& other){
		std::memcpy(bytes, other.bytes, 3);
	}

	bool operator==(const SquashedMacAddress& other){
		return std::memcmp(bytes,other.bytes,3) == 0;
	}

	/**
	 * Constructs a squashed mac address from a real mac addres.
	 *
	 * bytes = [mac[2*i] ^ mac[2*i+1] for i in range(3)]
	 */
	SquashedMacAddress(uint8_t *mac){
		bytes[0] = mac[0] ^ mac[1];
		bytes[1] = mac[2] ^ mac[3];
		bytes[2] = mac[4] ^ mac[5];
	}
};

/**
 *
 * TODO: channel?
 */
struct __attribute__((__packed__)) WitnessReport {
	SquashedMacAddress trackable;
	int8_t rssi;
	stone_id_t reporter;

	/**
	 * copy constructor enables assignment.
	 */
	WitnessReport(WitnessReport &other) :
			trackable(other.trackable),
			rssi(other.rssi),
			reporter(other.reporter) {
	}

	WitnessReport() = default;
};
