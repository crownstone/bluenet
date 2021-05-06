/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 17, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <protocol/cs_CuckooFilterStructs.h>
#include <protocol/cs_TrackableParserPackets.h>
#include <cstdint>

/**
 * Runtime meta data associated to a tracking_filter_t that is not persisted to flash.
 */
struct __attribute__((__packed__)) tracking_filter_runtime_data_t {
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
	 */
	uint16_t crc;
};
