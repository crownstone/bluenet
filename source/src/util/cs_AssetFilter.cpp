/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 27, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <util/cs_AssetFilter.h>

#include <logging/cs_Logger.h>
#include <util/cs_Utils.h>

#define LOGAssetFilterWarn LOGvv
#define LogLevelAssetFilteringVerbose SERIAL_VERY_VERBOSE


asset_filter_runtime_data_t* AssetFilter::runtimedata() {
	return reinterpret_cast<asset_filter_runtime_data_t*>(_data + 0);
}

AssetFilterData AssetFilter::filterdata() {
	return AssetFilterData(_data + sizeof(asset_filter_runtime_data_t));
}

// virtual functions

size_t AssetFilter::size() {
	return sizeof(asset_filter_runtime_data_t) + filterdata().length();
}

bool AssetFilter::isValid() {
	switch (*filterdata().metadata().filterType()) {
		case AssetFilterType::CuckooFilter: {
			return filterdata().cuckooFilter().isValid();
		}
		case AssetFilterType::ExactMatchFilter: {
			return filterdata().exactMatchFilter().isValid();
		}
		default: {
			return false;
		}
	}

	return false;
}

bool AssetFilter::contains(const void* key, size_t keyLengthInBytes)  {
	// switch on filter type in metadata and call contain.

	switch (*filterdata().metadata().filterType()) {
		case AssetFilterType::CuckooFilter: {
			return filterdata().cuckooFilter().contains(key, keyLengthInBytes);
		}
		case AssetFilterType::ExactMatchFilter: {
			return filterdata().exactMatchFilter().contains(key, keyLengthInBytes);
		}
		default: {
			return false;
		}
	}

	return false;
}



// ---------------------------- Extracting data from the filter  ----------------------------


template <class ReturnType, class ExpressionType>
ReturnType AssetFilter::prepareFilterInputAndCallDelegate(
		const scanned_device_t& device,
		AssetFilterInput filterInputDescription,
		ExpressionType delegateExpression,
		ReturnType defaultValue) {

	// obtain a pointer to an FilterInterface object of the correct filter type
	// (can be made prettier...)
	CuckooFilter cuckoo;     // Dangerous to use, it has a nullptr as data.
	ExactMatchFilter exact;  // Dangerous to use, it has a nullptr as data.
	FilterInterface* filter = nullptr;

	switch (*filterdata().metadata().filterType()) {
		case AssetFilterType::CuckooFilter: {
			cuckoo = filterdata().cuckooFilter();
			filter = &cuckoo;
			break;
		}
		case AssetFilterType::ExactMatchFilter: {
			exact  = filterdata().exactMatchFilter();
			filter = &exact;
			break;
		}
		default: {
			LOGAssetFilterWarn("Filter type not implemented");
			return defaultValue;
		}
	}

	// split out input type for the filter and prepare the input
	switch (*filterInputDescription.type()) {
		case AssetFilterInputType::MacAddress: {
			return delegateExpression(filter, device.address, sizeof(device.address));
		}
		case AssetFilterInputType::AdDataType: {
			// selects the first found field of configured type and checks if that field's
			// data is contained in the filter. returns defaultValue if it can't be found.
			cs_data_t result                  = {};
			ad_data_type_selector_t* selector = filterInputDescription.AdTypeField();

			if (selector == nullptr) {
				LOGe("Filter metadata type check failed");
				return defaultValue;
			}

			if (BLEutil::findAdvType(selector->adDataType, device.data, device.dataSize, &result) == ERR_SUCCESS) {
				return delegateExpression(filter, result.data, result.len);
			}

			return defaultValue;
		}
		case AssetFilterInputType::MaskedAdDataType: {
			// selects the first found field of configured type and checks if that field's
			// data is contained in the filter. returns false if it can't be found.
			cs_data_t result                         = {};
			masked_ad_data_type_selector_t* selector = filterInputDescription.AdTypeMasked();

			if (selector == nullptr) {
				LOGe("Filter metadata type check failed");
				return defaultValue;
			}

			if (BLEutil::findAdvType(selector->adDataType, device.data, device.dataSize, &result) == ERR_SUCCESS) {
				// A normal advertisement payload size is 31B at most.
				// We are also limited by the 32b bitmask.
				if (result.len > 31) {
					LOGw("Advertisement too large");
					return defaultValue;
				}
				uint8_t buff[31];

				// apply the mask
				uint8_t buffIndex = 0;
				for (uint8_t bitIndex = 0; bitIndex < result.len; bitIndex++) {
					if (BLEutil::isBitSet(selector->adDataMask, bitIndex)) {
						buff[buffIndex] = result.data[bitIndex];
						buffIndex++;
					}
				}
				_logArray(LogLevelAssetFilteringVerbose, true, buff, buffIndex);
				return delegateExpression(filter, buff, buffIndex);
			}

			return defaultValue;
		}
	}

	return defaultValue;
}

bool AssetFilter::filterAcceptsScannedDevice(const scanned_device_t& asset) {
	// The input result is nothing more than a call to .contains with the correctly prepared input.
	// It is 'correctly preparing the input' that is fumbly.
	return prepareFilterInputAndCallDelegate(
			asset,
			filterdata().metadata().inputType(),
			[](FilterInterface* filter, const uint8_t* data, size_t len) {
				return filter->contains(data, len);
			},
			false);
}

asset_id_t AssetFilter::getAssetId(const scanned_device_t& asset) {
	// The ouput result is nothing more than a call to .contains with the correctly prepared input.
	// It is 'correctly preparing the input' that is fumbly. (At least, if you don't want to always
	// preallocate the buffer that the MaskedAdData needs.)
	return prepareFilterInputAndCallDelegate(
			asset,
			filterdata().metadata().outputType().inFormat(),
			[this](FilterInterface* filter, const uint8_t* data, size_t len) {
				return assetId(data, len);
			},
			asset_id_t{});
}

asset_id_t AssetFilter::assetId(const void* key, size_t keyLengthInBytes) {
	auto crc = crc32(static_cast<const uint8_t*>(key), keyLengthInBytes, nullptr);

	// little endian byte by byte copy.
	asset_id_t id{};
	memcpy(id.data, reinterpret_cast<uint8_t*>(&crc), 3);

	return id;
}


