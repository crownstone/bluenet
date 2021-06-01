/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 27, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <logging/cs_Logger.h>
#include <util/cs_Error.h>
#include <util/cs_ExactMatchFilter.h>
#include <cstring>

ExactMatchFilter::ExactMatchFilter(exact_match_filter_data_t* data) : _data(data) {
	if (isValid() == false) {
		LOGe(false, "Invalid ExactMatchFilter encountered");
	}
}

bool ExactMatchFilter::isValid() {
	if (_data->itemCount == 0) {
		return false;
	}
	if (_data->itemSize == 0) {
		return false;
	}

	for (auto i = 0; i < _data->itemCount - 1; i++) {
		auto cmp = memcmp(getItem(i), getItem(i + 1), _data->itemSize);
		if (cmp > 0) {
			LOGe("Exact match filter requires sorted itemArray (index %u)", i);
			_logArray(SERIAL_DEBUG, true, getItem(i), _data->itemSize);
			_logArray(SERIAL_DEBUG, true, getItem(i+1), _data->itemSize);
			return false;
		}
	}

	return true;
}

bool ExactMatchFilter::contains(const void* key, size_t keyLengthInBytes) {
	if (keyLengthInBytes != _data->itemSize || _data->itemCount == 0) {
		return false;
	}

	// binary search. [lowerIndex, upperIndex] is
	// the inclusive candidate interval for the key index.
	// WARN: keep type equal to int, unsigned int will underflow when key
	// is smaller than smallest element in filter.
	int lowerIndex = 0;
	int upperIndex = _data->itemCount - 1;
	while (lowerIndex <= upperIndex) {
		size_t midpointIndex = (lowerIndex + upperIndex) / 2;
		auto cmp             = memcmp(key, getItem(midpointIndex), keyLengthInBytes);

		if (cmp == 0) {  // early return when found
			return true;
		}
		if (cmp > 0) {  // key > data[i]
			lowerIndex = midpointIndex + 1;
		}
		else {  // key < data[i]
			upperIndex = midpointIndex - 1;
		}
	}
	return false;
}

uint8_t* ExactMatchFilter::getItem(size_t index) {
	return _data->itemArray + index * _data->itemSize;
}

short_asset_id_t ExactMatchFilter::shortAssetId(const void* item, size_t itemSize) {
	// TODO what kind of short asset id are we going to use for this?
	return {};
}
