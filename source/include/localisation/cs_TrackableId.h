/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 20, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <algorithm> // for reverse_copy
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

	/**
	 * Note: mac bytes are stored in reverse order of how they
	 * are read on the nrf connect app.
	 */
	uint8_t bytes[SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	TrackableId() = default;

	/**
	 * mac address is copied as is into the .bytes member.
	 */
	TrackableId(const uint8_t* const mac, uint8_t len){
		if (len != SIZE) {
			LOGe("mac address of wrong size instantiated.");
		} else {
			std::copy(mac, mac + SIZE, bytes);
		}
	}

	/**
	 * Enables initialization like:
	 * TrackableId myId = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
	 * where the initializer_list is as read on the nrf connect app.
	 */
	TrackableId(const std::initializer_list<uint8_t> mac) :
		TrackableId(std::begin(mac), mac.size()) {
		std::reverse(bytes, bytes + SIZE);
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

	void print() {
		LOGd("tracked id mac=[%2x,%2x,%2x,%2x,%2x,%2x]",
				bytes[0],
				bytes[1],
				bytes[2],
				bytes[3],
				bytes[4],
				bytes[5]);
	}
};
