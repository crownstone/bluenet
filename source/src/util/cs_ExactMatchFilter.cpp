/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 27, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <cstring>
#include <util/cs_Error.h>
#include <util/cs_ExactMatchFilter.h>

ExactMatchFilter::ExactMatchFilter(exact_match_filter_data_t* data) : _data(data) {
	assert(_data->itemSize != 0, "itemSize cannot be zero");

	if (_data->itemCount == 0) {
		return;
	}

	for (auto i = 0; i < _data->itemCount - 1; i++) {
		auto cmp = memcmp(getItem(i), getItem(i + 1), _data->itemSize);
		if (cmp > 0) {
			assert(false, "Exact match filter requires sorted itemArray");
		}
	}
}

bool ExactMatchFilter::isValid() {
	if (_data->itemCount == 0) {
		return false;
	}
	if (_data->itemSize == 0) {
		return false;
	}
	// TODO: more checks?
	return true;
}

bool ExactMatchFilter::contains(const void* key, size_t keyLengthInBytes) {
	if (keyLengthInBytes != _data->itemSize || _data->itemCount == 0) {
		return false;
	}

	// binary search. [lowerIndex, upperIndex] is
	// the inclusive candidate interval for the key index.
	size_t lowerIndex = 0;
	size_t upperIndex = _data->itemCount - 1;
	while (true) {
		size_t midpointIndex  = (lowerIndex + upperIndex) / 2;
		auto cmp              = memcmp(key, getItem(midpointIndex), keyLengthInBytes);

		if (cmp == 0) {  // early return when found
			return true;
		}

		if (cmp > 0) {                          // key > data[i]
			if (midpointIndex == upperIndex) {  // reached upperIndex limit of interval
				return false;
			}
			lowerIndex = midpointIndex + 1;
		}
		else {                                  // key < data[i]
			if (midpointIndex == lowerIndex) {  // reached lowerIndex limit of interval
				return false;
			}
			upperIndex = midpointIndex - 1;
		}
	}
}

uint8_t* ExactMatchFilter::getItem(size_t index){
	return _data->itemArray + index * _data->itemSize;
}

short_asset_id_t ExactMatchFilter::shortAssetId(const void* item, size_t itemSize) {
// TODO what kind of short asset id are we going to use for this?
	return {};
}


