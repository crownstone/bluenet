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
#include <type_traits> // for assert(std::is_same ...) in binary search.

#define LogExactMatchFilterWarn LOGw

ExactMatchFilter::ExactMatchFilter(exact_match_filter_data_t* data) : _data(data) {
}

bool ExactMatchFilter::isValid() {
	if (_data == nullptr) {
		LogExactMatchFilterWarn("Data is nullptr.");
		return false;
	}
	if (_data->itemCount == 0) {
		LogExactMatchFilterWarn("itemCount can't be 0");
		return false;
	}
	if (_data->itemSize == 0) {
		LogExactMatchFilterWarn("itemSize can't be 0")
		return false;
	}

	for (auto i = 0; i < _data->itemCount - 1; i++) {
		auto cmp = memcmp(getItem(i), getItem(i + 1), _data->itemSize);
		if (cmp > 0) {
			LogExactMatchFilterWarn("Exact match filter requires sorted itemArray (index %u)", i);
			_logArray(SERIAL_DEBUG, true, getItem(i), _data->itemSize);
			_logArray(SERIAL_DEBUG, true, getItem(i+1), _data->itemSize);
			return false;
		}
	}

	return true;
}
int ExactMatchFilter::find(const void* item, size_t itemSize) {
	if (itemSize != _data->itemSize || _data->itemCount == 0) {
		return -1;
	}

	// Binary search. [lowerIndex, upperIndex] is the inclusive candidate interval for the key index.
	int lowerIndex = 0;
	int upperIndex = _data->itemCount - 1;

	static_assert(std::is_same<decltype(lowerIndex), int>() && std::is_same<decltype(upperIndex), int>(),
			"unsigned int will underflow when key is smaller than smallest element in filter");

	while (lowerIndex <= upperIndex) {
		int midpointIndex = (lowerIndex + upperIndex) / 2;
		auto cmp             = memcmp(item, getItem(midpointIndex), itemSize);

		if (cmp == 0) {  // early return when found
			return midpointIndex;
		}
		if (cmp > 0) {  // key > data[i]
			lowerIndex = midpointIndex + 1;
		}
		else {  // key < data[i]
			upperIndex = midpointIndex - 1;
		}
	}

	return -1;
}

bool ExactMatchFilter::contains(const void* key, size_t keyLengthInBytes) {
	return find(key,keyLengthInBytes) >= 0;
}

uint8_t* ExactMatchFilter::getItem(size_t index) {
	return _data->itemArray + index * _data->itemSize;
}

short_asset_id_t ExactMatchFilter::shortAssetId(const void* item, size_t itemSize) {
	int index = find(item,itemSize);
	if (index < 0) {
		// if unknown, use the default construction for SID

		return FilterInterface::shortAssetId(item, itemSize);
	}
	uint16_t indexUnsigned = static_cast<uint16_t>(index);

	short_asset_id_t id {
		.data{
			static_cast<uint8_t>((indexUnsigned >>  0) & 0xff),
			static_cast<uint8_t>((indexUnsigned >>  8) & 0xff),
			static_cast<uint8_t>((indexUnsigned >> 16) & 0xff)
		}
	};

	return id;
}
