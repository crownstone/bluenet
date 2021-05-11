/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 11, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

struct __attribute__((__packed__)) compressed_rssi_data_t {
	uint8_t channel : 2; // 0 = unknown, 1 = channel 37, 2 = channel 38, 3 = channel 39
	uint8_t rssi_halved : 6; // half of the absolute value of the original rssi.
};

inline compressed_rssi_data_t compressRssi(int8_t rssi, uint8_t channel = 0) {
	compressed_rssi_data_t compressed;
	switch (channel) {
		case 37: compressed.channel = 1;
		case 38: compressed.channel = 2;
		case 39: compressed.channel = 3;
		default: compressed.channel = 0;
	}

	if(rssi < 0) {
		rssi = -rssi;
	}

	compressed.rssi_halved = rssi/2;

	return compressed;

}
//
//compressed_rssi_t compressRssi(int8_t rssi) {
//
//}
//
//compressed_rssi_t compressChannel(uint8_t channel) {
//
//}
//
