/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 27, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#pragma once


#include <localisation/cs_AssetFilterPacketAccessors.h>

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

	/**
	 * Returns true if the device passes the filter according to its
	 * metadata settings. Returns false otherwise.
	 */
	bool filterAcceptsScannedDevice(AssetFilter filter, const scanned_device_t& asset);

private:
	/**
	 * This method extracts the filters 'input description', prepares the input according to that
	 * description and calls the delegate with the prepared data.
	 *
	 * delegateExpression should be of the form (FilterInterface&, void*, size_t) -> ReturnType.
	 *
	 * The argument that is passed into `delegateExpression` is based on the AssetFilterInputType
	 * of the `assetFilter`.Buffers are only allocated when strictly necessary.
	 * (E.g. MacAddress is already available in the `device`, but for MaskedAdDataType a buffer
	 * of 31 bytes needs to be allocated on the stack.)
	 *
	 * The delegate return type is left as free template parameter so that this template can be
	 * used for both `contains` and `assetId` return values.
	 */
	template <class ReturnType, class ExpressionType>
	ReturnType prepareFilterInputAndCallDelegate(
			AssetFilter assetFilter,
			const scanned_device_t& device,
			AssetFilterInput filterInputDescription,
			ExpressionType delegateExpression,
			ReturnType defaultValue);
};
