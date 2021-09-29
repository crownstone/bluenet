/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 17, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <protocol/cs_RssiAndChannel.h>

struct __attribute__((__packed__)) asset_record_t {
	/**
	 * ID of the asset, unique field.
	 */
	asset_id_t assetId;

	/**
	 * Most recent RSSI value observed by this Crownstone.
	 */
	rssi_and_channel_t myRssi;

	/**
	 * Set to 0 each time this asset is scanned.
	 * Increment at regular interval.
	 *
	 * Value of 0xFF marks the record invalid.
	 */
	uint8_t lastReceivedCounter = 0xFF;

	/**
	 * When not 0, no mesh message should be sent for this asset.
	 * Decrement at regular interval.
	 */
	uint8_t throttlingCountdown = 0;

#if BUILD_CLOSEST_CROWNSTONE_TRACKER == 1
	/**
	 * Stone id of the stone nearest to the asset,
	 * As always: stone ID of 0 is invalid.
	 */
	stone_id_t nearestStoneId;

	/**
	 * RSSI between asset and nearest stone.
	 * Only valid when the nearest stone ID is valid.
	 */
	int8_t nearestRssi;
#endif

	// ------------- utility functions -------------

	void empty() {
		lastReceivedCounter = 0;
		throttlingCountdown = 0;
#if BUILD_CLOSEST_CROWNSTONE_TRACKER == 1
		nearestStoneId = 0;
#endif
	}

	/**
	 * Invalidate this record.
	 */
	void invalidate() {
		lastReceivedCounter = 0xFF;
	}

	/**
	 * Returns whether this record is valid.
	 */
	bool isValid() {
		return lastReceivedCounter != 0xFF;
	}

	bool isThrottled() {
		return throttlingCountdown != 0;
	}

	void setThrottlingCountdown(uint8_t ticks) {
		if (ticks >= 0xff) {
			throttlingCountdown = 0xff - 1;
		} else {
			throttlingCountdown = ticks;
		}
	}

	void addThrottlingCountdown(uint8_t ticks) {
		auto val = uint16_t(throttlingCountdown) + ticks;
		setThrottlingCountdown(val);
	}
};
