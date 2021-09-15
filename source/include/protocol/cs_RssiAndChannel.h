/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 15, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#pragma once


struct __attribute__((__packed__)) rssi_and_channel_t {
	uint8_t channel : 2; // 0 = unknown, 1 = channel 37, 2 = channel 38, 3 = channel 39
	uint8_t rssiHalved : 6; // half of the absolute value of the original rssi.

	rssi_and_channel_t() = default;

	rssi_and_channel_t(int8_t rssi, uint8_t ch = 0) {
		switch (ch) {
			case 37: ch = 1; break;
			case 38: ch = 2; break;
			case 39: ch = 3; break;
			default: ch = 0; break;
		}

		if (rssi < 0) {
			rssi = -rssi;
		}
		else {
			// All positive values are compressed to 0.
			rssi = 0;
		}

		rssiHalved = rssi / 2;
	}

	uint8_t getChannel() const {
		return channel == 0 ? 0 : channel + 36;
	}

	int8_t getRssi() const {
		return -2 * rssiHalved;
	}

	bool isCloserThan(const rssi_and_channel_t& other) const {
		// usually higher values are shorter distances, but abs reverses inequality.
		return rssiHalved < other.rssiHalved;
	}

	bool isCloserEqual(const rssi_and_channel_t& other) const {
		return isCloserThan(other) || rssiHalved == other.rssiHalved;
	}
};
