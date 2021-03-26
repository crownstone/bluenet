/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 17, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cstdint>
#include <protocol/cs_TrackableParserPackets.h>
#include <third/cuckoo/CuckooFilter.h>

/**
 * Data associated to a parsing filter that is not persisted to flash.
 */
struct __attribute__((__packed__)) tracking_filter_runtime_data_t {
	uint8_t filterId; 				// can be runtime only since it's saved as part of record metadata on flash.
	uint16_t crc;
};


/**
 * The filters and their associated metadata.
 * Datatype is used in the filter object pool
 *
 * Warning: keep this structure and order as-is, that makes chunking
 * the whole thing a stupid load simpler.
 */
struct __attribute__((__packed__)) tracking_filter_t {
	tracking_filter_runtime_data_t  runtimedata;
	tracking_filter_meta_data_t metadata;
	cuckoo_filter_data_t filterdata;

	size_t size() {
		return sizeof(tracking_filter_t) + CuckooFilter(filterdata).bufferSize();
	}
};
