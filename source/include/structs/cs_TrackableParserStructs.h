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
	 *
	 *
	 * can be runtime only since it's saved as part of record metadata on flash.
	 * */
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
//
///**
// * The filters and their associated metadata.
// * Datatype is used in the filter object pool
// *
// * Warning: keep this structure and order as-is, that makes chunking
// * the whole thing a stupid load simpler.
// */
//struct __attribute__((__packed__)) tracking_filter_t {
//	tracking_filter_runtime_data_t runtimedata;
//	tracking_filter_meta_data_t metadata;
//	cuckoo_filter_data_t filterdata;
//};
