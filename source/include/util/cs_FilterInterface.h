/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 27, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <protocol/cs_AssetFilterPackets.h>

class FilterInterface {
public:
	virtual ~FilterInterface() = default;

	virtual bool contains(const void* key, size_t keyLengthInBytes) = 0;

	virtual short_asset_id_t shortAssetId(const void* key, size_t keyLengthInBytes) = 0;

	virtual size_t size() = 0;

	// shortAssetId(key, len) = 0
	virtual bool isValid() = 0;
};
