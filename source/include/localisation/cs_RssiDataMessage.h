/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 9, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdint>


/**
 * The data in this packet contains information about the
 * bluetooth channels between this crownstone and the one
 * with id sender_id.
 */
struct __attribute__((__packed__)) rssi_data_message_t {
	stone_id_t sender_id;

	/**
	 * a samplecount == 0x111111, indicates the channel had
	 * at least 2^6-1 == 63 samples.
	 */
	uint8_t sample_count_ch37 : 6;
	uint8_t sample_count_ch38 : 6;
	uint8_t sample_count_ch39 : 6;

	/**
	 * absolute value of the average rssi
	 */
	uint8_t rssi_ch37 : 7;
	uint8_t rssi_ch38 : 7;
	uint8_t rssi_ch39 : 7;

	/**
	 * variance of the given channel, rounded to intervals:
	 * 0: 0  - 2
	 * 1: 2  - 4
	 * 2: 4  - 8
	 * 3: 8  - 12
	 * 4: 12 - 16
	 * 5: 16 - 24
	 * 6: 24 - 32
	 * 7: 32 and over
	 */
	uint8_t variance_ch37 : 3;
	uint8_t variance_ch38 : 3;
	uint8_t variance_ch39 : 3;

};
