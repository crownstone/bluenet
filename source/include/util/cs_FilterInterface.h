/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 27, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <protocol/cs_AssetFilterPackets.h>
#include <util/cs_Crc32.h>


/**
 * Used in AssetFiltering as a generic way to query a filter for containment and shortAssetId.
 */
class FilterInterface {
public:
	virtual ~FilterInterface() = default;

	virtual bool contains(const void* key, size_t keyLengthInBytes) = 0;

	/**
	 * In most cases a shortAssetId is generated as crc32 from filtered input data.
	 * This can be overwritten for special use cases like ExactMatchFilter.
	 */
	virtual short_asset_id_t shortAssetId(const void* key, size_t keyLengthInBytes) {
		auto crc = crc32(static_cast<const uint8_t*>(key), keyLengthInBytes, nullptr);

		// REVIEW: isn't this the same as:
		// crc = crc & 0x00FFFFFF;
		// memcpy(data, crc, 3);
		// or even: memcpy(data, crc+1, 3);
		short_asset_id_t id{
			.data{
				static_cast<uint8_t>((crc >>  0) & 0xFF),
				static_cast<uint8_t>((crc >>  8) & 0xFF),
				static_cast<uint8_t>((crc >> 16) & 0xFF),
			}
		};

		return id;
	}

	virtual size_t size() = 0;

	virtual bool isValid() = 0;
};
