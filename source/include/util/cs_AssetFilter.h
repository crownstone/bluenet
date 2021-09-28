/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 27, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#pragma once


#include <localisation/cs_AssetFilterPacketAccessors.h>
#include <util/cs_FilterInterface.h>
#include <structs/cs_PacketsInternal.h>

/**
 * Class that contains all data required for an asset filter:
 * - Runtime data.
 * - persisted data (filterdata).
 */
class AssetFilter : FilterInterface {
public:
	uint8_t* _data;  // byte representation of this object.
	AssetFilter(uint8_t* data) : _data(data) {}

	// ================
	// Accessor methods
	// ================

	/**
	 * Get the runtime data.
	 */
	asset_filter_runtime_data_t* runtimedata();

	/**
	 * Get the filter data.
	 */
	AssetFilterData filterdata();

	// ===============
	// FilterInterface
	// ===============

	/**
	 * Number of bytes of the data, according to the metadata contained in it.
	 */
	size_t size() override;

	bool contains(const void* key, size_t keyLengthInBytes) override;

	bool isValid() override;


	// ================
	// Scan based utils
	// ================

	/**
	 * Returns true if the device passes the filter according to its
	 * metadata settings. Returns false otherwise.
	 */
	bool filterAcceptsScannedDevice(const scanned_device_t& asset);

	/**
	 * Returns a asset_id_t based on the configured selection of data
	 * in metadata.outputType.inFormat. If the data is not sufficient, a default
	 * constructed object is returned. (Data not sufficient can be detected:
	 * filterInputResult will return false in that case.)
	 */
	asset_id_t getAssetId(const scanned_device_t& asset);


private:
	/**
	 * A assetId is generated as crc32 from filtered input data.
	 *
	 * For output type MAC the data is prepended with a fixed
	 * value to avoid collisions with filters with assetId output based
	 * on MAC.
	 */
	virtual asset_id_t assetId(const void* key, size_t keyLengthInBytes, uint32_t* startCrc = nullptr);

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
			const scanned_device_t& device,
			AssetFilterInput filterInputDescription,
			ExpressionType delegateExpression,
			ReturnType defaultValue);
};
