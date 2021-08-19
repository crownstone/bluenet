/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 17, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <util/cs_Rssi.h>

struct __attribute__((__packed__)) asset_record_t {

	short_asset_id_t assetId;

	/**
	 * Most recent rssi value observed by this crownstone.
	 */

	compressed_rssi_data_t myRssi;

#if BUILD_CLOSEST_CROWNSTONE_TRACKER == 1
	/**
	 * rssi between asset and nearest stone. only valid if winning_id != 0.
	 */
	compressed_rssi_data_t nearestRssi;

	/**
	 * Stone id of currently nearest stone.
	 *  - Equal to 0 if unknown
	 *  - Equal to _myId if this crownstone believes it's itself
	 *    the nearest stone to this asset.
	 */
	stone_id_t nearestStoneId;
#endif


	uint8_t timeSinceLastReceivedUpdate;
	uint8_t timeSinceLastSentUpdate;

	// ------------- utility functions -------------

	/**
	 * Empty asset records have (compressed) rssi values of -127 set
	 * rather than 0 to ensure 'empty' is 'far away'.
	 */
	static asset_record_t empty() {
		asset_record_t rec = {};
		rec.myRssi = compressRssi(-127,0);

#if BUILD_CLOSEST_CROWNSTONE_TRACKER == 1
		rec.nearestRssi = compressRssi(-127, 0);
#endif

		return rec;
	}
};
