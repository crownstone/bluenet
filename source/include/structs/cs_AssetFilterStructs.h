/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 17, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdint>
#include <protocol/cs_AssetFilterPackets.h>
#include <protocol/cs_CuckooFilterStructs.h>

/**
 * Runtime meta data associated to an asset filter that is not persisted to flash.
 */
struct __attribute__((__packed__)) asset_filter_runtime_data_t {
	/**
	 * Id of a filter.
	 *
	 * Can be runtime only since it's saved as part of record metadata on flash. (State Id equals filter Id)
	 */
	uint8_t filterId;

	/**
	 * The size of the tracking_filter_t, including the buffer, excluding sizeof(runtimedata).
	 * i.e.:
	 * - sizeof(metadata)
	 * - sizeof(filterdata)
	 * - fingerprint array size of the filterdata
	 *
	 * can be runtime only since it's saved as part of record metadata on flash.
	 */
	uint16_t totalSize;

	/**
	 * crc16 of the fields:
	 * - tracking_filter_t::metadata
	 * - tracking_filter_t::filterdata
	 *
	 * The value 0 wil be interpreted as 'not valid'.
	 * TODO: a CRC of 0 is a valid case.
	 */
	uint16_t crc;
};
