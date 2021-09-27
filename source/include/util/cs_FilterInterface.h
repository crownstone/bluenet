/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 27, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <localisation/cs_AssetFilterPacketAccessors.h>
#include <protocol/cs_AssetFilterPackets.h>
#include <util/cs_Crc32.h>


/**
 * Used in AssetFiltering as a generic way to query a filter for containment and assetId.
 */
class FilterInterface {
public:
	virtual ~FilterInterface() = default;

	virtual bool contains(const void* key, size_t keyLengthInBytes) = 0;

	/**
	 * In most cases a assetId is generated as crc32 from filtered input data.
	 * This can be overwritten for special use cases like ExactMatchFilter.
	 */
	virtual asset_id_t assetId(const void* key, size_t keyLengthInBytes);
	virtual size_t size() = 0;

	virtual bool isValid() = 0;


};
