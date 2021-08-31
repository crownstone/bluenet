/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 11, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <protocol/mesh/cs_MeshModelPackets.h>

// REVIEW: can't we add these functions to compressed_rssi_data_t ?

// REVIEW: why inline?
inline constexpr compressed_rssi_data_t compressRssi(int8_t rssi, uint8_t channel = 0) {
	compressed_rssi_data_t compressed = {};
	switch (channel) {
		case 37: compressed.channel = 1; break;
		case 38: compressed.channel = 2; break;
		case 39: compressed.channel = 3; break;
		default: compressed.channel = 0; break;
	}

	if (rssi < 0) {
		rssi = -rssi;
	}
	else {
		// All positive values are compressed to 0.
		rssi = 0;
	}

	compressed.rssiHalved = rssi / 2;

	return compressed;
}

inline uint8_t getChannel(const compressed_rssi_data_t& compressed) {
	return compressed.channel == 0 ? 0 : compressed.channel + 36;
}

inline int8_t getRssi(const compressed_rssi_data_t& compressed) {
	return -2 * compressed.rssiHalved;
}

/**
 * Returns true if the rssi value of lhs represents a shorter
 * physical distance than rhs. Ignores channel.
 */
inline bool rssiIsCloser(
		const compressed_rssi_data_t& lhs,
		const compressed_rssi_data_t& rhs) {
	// usually higher values are shorter distances, but abs reverses inequality.
	return lhs.rssiHalved < rhs.rssiHalved;
}

inline bool rssiIsCloserEqual(
		const compressed_rssi_data_t& lhs,
		const compressed_rssi_data_t& rhs) {
	return rssiIsCloser(lhs, rhs) || lhs.rssiHalved == rhs.rssiHalved;
}
