/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 27, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <protocol/cs_ExactMatchFilterStructs.h>
#include <util/cs_FilterInterface.h>

class ExactMatchFilter : public FilterInterface {
public:
	// -------------------------------------------------------------
	// Interface methods.
	// -------------------------------------------------------------

	ExactMatchFilter(exact_match_filter_data_t* data);

	/**
	 * Use this default constructor with care, _data is never checked in this class.
	 */
	ExactMatchFilter() : _data(nullptr){}

	bool contains(const void* key, size_t keyLengthInBytes) override;

	/**
	 * A short asset id for an exact match filter is formatted as
	 * 	- { index_lsb, index_msb, 0xff} when item is present
	 *  - { 0xff, 0xff, 0xff} when item is not present.
	 */
	short_asset_id_t shortAssetId(const void* item, size_t itemSize) override;

	bool isValid() override;

	/**
	 * if contains(key,itemSize): returns the index of the item
	 * else: return -1
	 */
	int find(const void* item, size_t itemSize);

	// -------------------------------------------------------------
	// Sizing helpers
	// -------------------------------------------------------------

	/**
	 * Size of the itemArray in bytes.
	 */
	static constexpr size_t bufferSize(uint8_t itemCount, uint8_t itemSize) {
		return itemCount * itemSize;
	}

	/**
	 * Total number of bytes that a filter with the given parameters would occupy.
	 */
	static constexpr size_t size(uint8_t itemCount, uint8_t itemSize) {
		return sizeof(exact_match_filter_data_t) + bufferSize(itemCount, itemSize);
	}

	constexpr size_t bufferSize() {
		return bufferSize(_data->itemCount, _data->itemSize);
	}

	constexpr size_t size() {
		return size(_data->itemCount, _data->itemSize);
	}

private:
	uint8_t* getItem(size_t index);

	exact_match_filter_data_t* _data;

};
