/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 7, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <localisation/cs_AssetFiltering.h>
#include <util/cs_Utils.h>

#define LOGAssetFilteringWarn LOGw
#define LOGAssetFilteringInfo LOGi
#define LOGAssetFilteringDebug LOGd
#define LogLevelAssetFilteringDebug   SERIAL_VERY_VERBOSE
#define LogLevelAssetFilteringVerbose SERIAL_VERY_VERBOSE

cs_ret_code_t AssetFiltering::init() {
	LOGAssetFilteringInfo("init");
	cs_ret_code_t retCode = ERR_UNSPECIFIED;

	// TODO: it seems there's a bunch of code to make multiple calls to init() possible,
	// though listen() now can be called multiple times.
	// Do we even want to support this?

	// TODO: cleanup all if init fails?

	if (_filterStore == nullptr) {
		_filterStore = new AssetFilterStore();
		if (_filterStore == nullptr) {
			return ERR_NO_SPACE;
		}
	}
	retCode = _filterStore->init();
	if (retCode != ERR_SUCCESS) {
		return retCode;
	}

	if (_filterSyncer == nullptr) {
		_filterSyncer = new AssetFilterSyncer();
		if (_filterSyncer == nullptr) {
			return ERR_NO_SPACE;
		}
	}
	retCode = _filterSyncer->init(*_filterStore);
	if (retCode != ERR_SUCCESS) {
		return retCode;
	}

	if (_assetForwarder == nullptr) {
		_assetForwarder = new AssetForwarder();
		if (_assetForwarder == nullptr) {
			return ERR_NO_SPACE;
		}
	}
	retCode = _assetForwarder->init();
	if (retCode != ERR_SUCCESS) {
		return retCode;
	}

	listen();
	return ERR_SUCCESS;
}

void AssetFiltering::setAssetHandlerMac(AssetHandlerMac* assetHandlerMac) {
	_assetHandlerMac = assetHandlerMac;
}

void AssetFiltering::setAssetHandlerShortId(AssetHandlerShortId* assetHandlerShortId) {
	_assetHandlerShortId = assetHandlerShortId;
}

// ---------------------------- Handling events ----------------------------

bool AssetFiltering::isInitialized() {
	return _assetForwarder != nullptr && _filterSyncer != nullptr && _filterStore != nullptr;
}

void AssetFiltering::handleEvent(event_t& evt) {
	if (!isInitialized()) {
		return;
	}

	switch (evt.type) {
		case CS_TYPE::EVT_DEVICE_SCANNED: {
			auto scannedDevice = CS_TYPE_CAST(EVT_DEVICE_SCANNED, evt.data);
			handleScannedDevice(*scannedDevice);
			break;
		}
		default:
			break;
	}
}

void AssetFiltering::handleScannedDevice(const scanned_device_t& device) {
	if (!_filterStore->isReady()) {
		return;
	}

	for (uint8_t i = 0; i < _filterStore->getFilterCount(); ++i) {
		auto filter = AssetFilter(_filterStore->getFilter(i));
		if (filterInputResult(filter, device)) {
			LOGAssetFilteringDebug("FilterId=%u accepted device with mac: %02X:%02X:%02X:%02X:%02X:%02X",
					filter.runtimedata()->filterId,
					device.address[5],
					device.address[4],
					device.address[3],
					device.address[2],
					device.address[1],
					device.address[0]);
			_logArray(LogLevelAssetFilteringDebug, true, device.data, device.dataSize);
			processAcceptedAsset(filter, device);
		}
	}
}

void AssetFiltering::processAcceptedAsset(AssetFilter filter, const scanned_device_t& asset) {
	switch (*filter.filterdata().metadata().outputType().outFormat()) {
		case AssetFilterOutputFormat::Mac: {
			if (_assetHandlerMac != nullptr) {
				_assetHandlerMac->handleAcceptedAsset(filter, asset);
			}

			if (_assetForwarder != nullptr) {
				_assetForwarder->handleAcceptedAsset(filter, asset);
			}

			break;
		}

		case AssetFilterOutputFormat::ShortAssetId: {
			if (_assetHandlerShortId != nullptr) {
				short_asset_id_t shortId = filterOutputResultShortAssetId(filter, asset);
				_assetHandlerShortId->handleAcceptedAsset(filter, asset, shortId);
			}
			break;
		}
	}
}



// ---------------------------- Extracting data from the filter  ----------------------------

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
 * used for both `contains` and `shortAssetId` return values.
 */
template <class ReturnType, class ExpressionType>
ReturnType prepareFilterInputAndCallDelegate(
		AssetFilter assetFilter,
		const scanned_device_t& device,
		AssetFilterInput filterInputDescription,
		ExpressionType delegateExpression,
		ReturnType defaultValue) {

	// obtain a pointer to an FilterInterface object of the correct filter type
	// (can be made prettier...)
	CuckooFilter cuckoo;     // Dangerous to use, it has a nullptr as data.
	ExactMatchFilter exact;  // Dangerous to use, it has a nullptr as data.
	FilterInterface* filter = nullptr;

	switch (*assetFilter.filterdata().metadata().filterType()) {
		case AssetFilterType::CuckooFilter: {
			cuckoo = assetFilter.filterdata().cuckooFilter();
			filter = &cuckoo;
			break;
		}
		case AssetFilterType::ExactMatchFilter: {
			exact  = assetFilter.filterdata().exactMatchFilter();
			filter = &exact;
			break;
		}
		default: {
			LOGAssetFilteringWarn("Filter type not implemented");
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

			assert(selector != nullptr, "Filter metadata type check failed");

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

			assert(selector != nullptr, "Filter metadata type check failed");

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

bool AssetFiltering::filterInputResult(AssetFilter assetFilter, const scanned_device_t& asset) {
	// The input result is nothing more than a call to .contains with the correctly prepared input.
	// It is 'correctly preparing the input' that is fumbly.
	return prepareFilterInputAndCallDelegate(
			assetFilter,
			asset,
			assetFilter.filterdata().metadata().inputType(),
			[](FilterInterface* filter, const uint8_t* data, size_t len) {
				return filter->contains(data, len);
			},
			false);
}

short_asset_id_t AssetFiltering::filterOutputResultShortAssetId(AssetFilter assetFilter, const scanned_device_t& asset) {
	// The ouput result is nothing more than a call to .contains with the correctly prepared input.
	// It is 'correctly preparing the input' that is fumbly. (At least, if you don't want to always
	// preallocate the buffer that the MaskedAdData needs.)
	return prepareFilterInputAndCallDelegate(
			assetFilter,
			asset,
			assetFilter.filterdata().metadata().outputType().inFormat(),
			[](FilterInterface* filter, const uint8_t* data, size_t len) {
				return filter->shortAssetId(data, len);
			},
			short_asset_id_t{});
}
