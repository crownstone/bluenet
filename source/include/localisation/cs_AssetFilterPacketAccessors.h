/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 5, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <protocol/cs_AssetFilterPackets.h>
#include <structs/cs_AssetFilterStructs.h>
#include <util/cs_CuckooFilter.h>
#include <util/cs_ExactMatchFilter.h>

/**
 * This file defines several accessor classes for structures that
 * relate to the trackable parser. They are constructed using a buffer pointer
 * and provide read/write access to the buffer according to the protocol.
 */


class AssetFilterInput {
public:
	uint8_t* _data;  // byte representation of this object.

	AssetFilterInput(uint8_t* data) : _data(data){}

	AssetFilterInputType*           type();
	ad_data_type_selector_t*        AdTypeField();  // returns nullptr unless *type() == AdDataType
	masked_ad_data_type_selector_t* AdTypeMasked(); // returns nullptr unless *type() == MaskedAdDataType

	/**
	 * Total length of the AdvertisementSubdata. Depends on type().
	 */
	size_t length();
};

class AssetFilterOutput {
public:
	uint8_t* _data;

	AssetFilterOutput(uint8_t* data) : _data(data) {}

	AssetFilterOutputFormat* outFormat();
	AssetFilterInput         inFormat(); // invalid unless *outFormat() == ShortAssetId

	size_t length();
};

class AssetFilterMetadata {
public:
	uint8_t* _data;  // byte representation of this object.

	AssetFilterMetadata(uint8_t* data) : _data(data) {}

	AssetFilterType*  filterType();
	uint8_t*          profileId();
	AssetFilterInput  inputType();
	AssetFilterOutput outputType();

	size_t length();
};

class AssetFilterData {
public:
	uint8_t* _data;  // byte representation of this object.
	AssetFilterData(uint8_t* data) : _data(data) {}

	AssetFilterMetadata metadata();
	CuckooFilter cuckooFilter(); // invalid unless *metadata().type() == FilterType::CukooFilter
	ExactMatchFilter exactMatchFilter(); // invalid unless *metadata().type() == FilterType::ExactMatchFilter

	bool isValid();
	size_t length();
};

class AssetFilter {
public:
	uint8_t* _data;  // byte representation of this object.

	AssetFilter(uint8_t* data) : _data(data) {}

	asset_filter_runtime_data_t* runtimedata();
	AssetFilterData filterdata();

	/**
	 * Total size ot the _data, including runtimedata() and the cuckooFilter().
	 *
	 * I.e. computed from the runtimedata() length and cuckooFilter().length().
	 */
	size_t length();
};
