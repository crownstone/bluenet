/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 15, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <util/cs_Math.h>

// ---------------------------------------------------------------------

uint8_t inline compressChannel(uint8_t channel) {
	switch (channel) {
		case 37: return 1;
		case 38: return 2;
		case 39: return 3;
		default: return 0;
	}
}

uint8_t inline decompressChannel(uint8_t compressedChannel) {
	return compressedChannel == 0 ? 0 : compressedChannel + 36;
}

uint8_t inline compressRssi(int8_t rssi) {
	if (rssi < 0) {
		rssi = -rssi;
	}
	else {
		// All positive values are compressed to 0.
		rssi = 0;
	}

	return rssi / 2;
}

int8_t inline decompressRssi(uint8_t compressedRssi) {
	return -2 * compressedRssi;
}

// ---------------------------------------------------------------------

class rssi_and_channel_float_t {
public:
	float _rssi;       // signed rssi, usually negative. higher is closer.
	uint8_t _channel;  // 37-39

	rssi_and_channel_float_t(float rssi, uint8_t channel = 0) : _rssi(rssi), _channel(channel) {}

	bool isCloserThan(const rssi_and_channel_float_t& other) { return _rssi > other._rssi; }

	bool isCloserEqual(const rssi_and_channel_float_t& other) { return isCloserThan(other) || _rssi == other._rssi; }

	rssi_and_channel_float_t fallOff(float rate_db_sec, float dt_sec) {
		return rssi_and_channel_float_t(_rssi - rate_db_sec * dt_sec, _channel);
	}
};

// ---------------------------------------------------------------------

struct __attribute__((__packed__)) rssi_and_channel_t {
	uint8_t channel : 2;     // 0 = unknown, 1 = channel 37, 2 = channel 38, 3 = channel 39
	uint8_t rssiHalved : 6;  // half of the absolute value of the original rssi.

	rssi_and_channel_t() = default;

	rssi_and_channel_t(int8_t rssi, uint8_t ch = 0) {
		channel    = compressChannel(ch);
		rssiHalved = compressRssi(rssi);
	}

	uint8_t getChannel() const { return decompressChannel(channel); }

	int8_t getRssi() const { return decompressRssi(rssiHalved); }

	rssi_and_channel_float_t toFloat() { return rssi_and_channel_float_t(getRssi(), getChannel()); }

	rssi_and_channel_float_t fallOff(float rate_db_sec, float dt_sec) { return toFloat().fallOff(rate_db_sec, dt_sec); }
};
