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
	 * Stone id of currently closest stone.
	 *  - Equal to 0 if unknown
	 *  - Equal to _myId if this crownstone believes it's itself
	 *    the closest stone to this asset.
	 */
	stone_id_t closestStoneId;

	/**
	 * Most recent rssi value observed by this crownstone.
	 */

	compressed_rssi_data_t myRssi;
	/**
	 * rssi between asset and closest stone. only valid if winning_id != 0.
	 */

	compressed_rssi_data_t closestRssi;

	// ------------- utility functions -------------

	/**
	 * Empty asset records have (compressed) rssi values of -127 set
	 * rather than 0 to ensure 'empty' is 'far away'.
	 */
	static asset_record_t empty() {
		asset_record_t rec = {};
		rec.myRssi = compressRssi(-127,0);
		rec.closestRssi = compressRssi(-127, 0);
		return rec;
	}
};
