/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 11, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <protocol/mesh/cs_MeshModelPackets.h>

inline compressed_rssi_data_t compressRssi(int8_t rssi, uint8_t channel = 0) {
	compressed_rssi_data_t compressed;
	switch (channel) {
		case 37: compressed.channel = 1; break;
		case 38: compressed.channel = 2; break;
		case 39: compressed.channel = 3; break;
		default: compressed.channel = 0; break;
	}

	if (rssi < 0) {
		rssi = -rssi;
	}

	compressed.rssi_halved = rssi / 2;

	return compressed;
}

inline uint8_t getChannel(const compressed_rssi_data_t& compressed) {
	return compressed.channel == 0 ? 0 : compressed.channel + 36;
}

inline int8_t getRssi(const compressed_rssi_data_t& compressed) {
	return -2 * compressed.rssi_halved;
}

inline uint8_t getRssiUnsigned(const compressed_rssi_data_t& compressed) {
	return 2 * compressed.rssi_halved;
}
