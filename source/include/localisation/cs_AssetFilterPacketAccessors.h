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


/**
 * Class that determines the input data for a filter or for the output.
 */
class AssetFilterInput {
public:
	uint8_t* _data;  // byte representation of this object.
	AssetFilterInput(uint8_t* data) : _data(data) {}

	/**
	 * Get the type of input.
	 */
	AssetFilterInputType*           type();

	/**
	 * Get the payload for type AdDataType.
	 *
	 * @return nullptr if type is not AdDataType.
	 */
	ad_data_type_selector_t*        AdTypeField();

	/**
	 * Get the payload for type MaskedAdDataType.
	 *
	 * @return nullptr if type is not MaskedAdDataType.
	 */
	masked_ad_data_type_selector_t* AdTypeMasked();

	/**
	 * Checks if the data has valid values.
	 */
	bool isValid();

	/**
	 * Get the expected length of this class, depends on type.
	 */
	size_t length();
};

/**
 * Class that determines the output method and data of an asset that passed the filter.
 */
class AssetFilterOutput {
public:
	uint8_t* _data;
	AssetFilterOutput(uint8_t* data) : _data(data) {}

	/**
	 * Get the type of output.
	 */
	AssetFilterOutputFormat* outFormat();

	/**
	 * Get the input format.
	 *
	 * Currently only valid if type is ShortAssetId.
	 */
	AssetFilterInput         inFormat();

	/**
	 * Checks if the data has valid values.
	 */
	bool isValid();

	/**
	 * Get the expected length of this class, depends on type.
	 */
	size_t length();
};

/**
 * Class that contains all persisted metadata required for an asset filter:
 * - Type of filter.
 * - Flags.
 * - What profile ID this filter is associated with.
 * - What data is used as input for the filter.
 * - What the output should be.
 */
class AssetFilterMetadata {
public:
	uint8_t* _data;  // byte representation of this object.
	AssetFilterMetadata(uint8_t* data) : _data(data) {}

	/**
	 * Get the type of filter.
	 */
	AssetFilterType* filterType();

	/**
	 * Get the flags.
	 */
	asset_filter_flags_t* flags();

	/**
	 * Get the associated profile ID.
	 */
	uint8_t* profileId();

	/**
	 * Get the input class.
	 */
	AssetFilterInput inputType();

	/**
	 * Get the output class.
	 */
	AssetFilterOutput outputType();

	/**
	 * Checks if the data has valid values.
	 */
	bool isValid();

	/**
	 * Get the expected length of this class.
	 */
	size_t length();
};

/**
 * Class that contains all persisted data required for an asset filter:
 * - Metadata.
 * - Filter data.
 */
class AssetFilterData {
public:
	uint8_t* _data;  // byte representation of this object.
	AssetFilterData(uint8_t* data) : _data(data) {}

	/**
	 * Get the metadata.
	 */
	AssetFilterMetadata metadata();

	/**
	 * Get the cuckoo filter.
	 *
	 * Only valid if filter type is FilterType::CukooFilter.
	 */
	CuckooFilter cuckooFilter();

	/**
	 * Get the cuckoo filter.
	 *
	 * Only valid if filter type is FilterType::ExactMatchFilter.
	 */
	ExactMatchFilter exactMatchFilter();

	/**
	 * Checks if the data has valid values.
	 */
	bool isValid();

	/**
	 * Get the expected length of this class.
	 */
	size_t length();
};

/**
 * Class that contains all data required for an asset filter:
 * - Runtime data.
 * - persisted data (filterdata).
 */
class AssetFilter {
public:
	uint8_t* _data;  // byte representation of this object.
	AssetFilter(uint8_t* data) : _data(data) {}

	/**
	 * Get the runtime data.
	 */
	asset_filter_runtime_data_t* runtimedata();

	/**
	 * Get the filter data.
	 */
	AssetFilterData filterdata();

	/**
	 * Get the expected length of this class.
	 */
	size_t length();
};
