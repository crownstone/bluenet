/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 3, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <stdint.h>
#include <third/cuckoo/CuckooFilter.h>


// -------------------------------------------------------------
// --------------- Filter definitions and storage --------------
// -------------------------------------------------------------

/**
 * Data associated to a parsing filter that is persisted to flash.
 */
struct __attribute__((__packed__)) FilterMetaData {
public:
	// sync info
	uint8_t protocol; // determines implementation type of filter
	union {
		struct {
			uint16_t value: 12; // Lollipop
			uint16_t unused : 4; // explicit padd
		} version;
		uint16_t version_int;
	};

	uint8_t profileId; // devices passing the filter are assigned this profile id
	uint8_t inputType; // determines wether the filter has mac or advertisement data as input

	// sync state
	union{
		uint8_t flags;
		struct {
			uint8_t isActive : 1;
		} bits;
	};
};

/**
 * Data associated to a parsing filter that is not persisted to flash.
 */
struct __attribute__((__packed__)) FilterRuntimeData {
	uint8_t filterId; 				// can be runtime only since it's saved as part of record metadata on flash.
	uint16_t crc;
};

/**
 * The filters and their associated metadata.
 * Allocated on the FilterBuffer are packed into a single
 * struct. That saves a bit of administration.
 *
 * Warning: keep this structure and order as-is, that makes chunking
 * the whole thing a stupid load simpler.
 */
struct __attribute__((__packed__)) ParsingFilter {
	FilterRuntimeData runtimedata;
	FilterMetaData metadata; 		// must be kept next to cuckoo filter to make copy to flash a simple memcpy.
	CuckooFilter filter;			// must be last item to give space to flexible array

	size_t size() {
		return sizeof(ParsingFilter) + filter.bufferSize();
	}
};
