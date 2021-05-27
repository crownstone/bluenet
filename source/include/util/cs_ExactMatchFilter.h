/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 27, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <protocol/cs_ExactMatchFilterStructs.h>
#include <util/cs_IFilter.h>

class ExactMatchFilter : public IFilter {
public:
	/**
	 * Wraps a data struct into a Filter object
	 */
	ExactMatchFilter(exact_match_filter_data_t& data);

	bool contains(filter_entry_t key, size_t keyLengthInBytes);



	// -------------------------------------------------------------
	// Sizing helpers
	// -------------------------------------------------------------

	/**
	 * Size of the itemArray in bytes.
	 */
	static constexpr size_t bufferSize(uint16_t itemCount, uint16_t itemSize) {
		return itemCount * itemSize;
	}

	/**
	 * Total number of bytes that a filter with the given parameters would occupy.
	 */
	static constexpr size_t size(uint16_t itemCount, uint16_t itemSize) {
		return sizeof(exact_match_filter_data_t) + bufferSize(itemCount, itemSize);
	}

	constexpr size_t bufferSize() {
		return bufferSize(_data.itemCount, _data.itemSize);
	}

	constexpr size_t size() {
		return size(_data.itemCount, _data.itemSize);
	}

private:
	uint8_t* getItem(size_t index);

	exact_match_filter_data_t& _data;

};
