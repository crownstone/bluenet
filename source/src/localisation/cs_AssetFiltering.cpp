/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 7, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <localisation/cs_AssetFiltering.h>
#include <util/cs_Utils.h>

#define LOGAssetFilteringWarn LOGw
#define LOGAssetFilteringInfo LOGvv
#define LOGAssetFilteringDebug LOGvv
#define LogLevelAssetFilteringDebug   SERIAL_VERY_VERBOSE
#define LogLevelAssetFilteringVerbose SERIAL_VERY_VERBOSE


void LogAcceptedDevice(AssetFilter filter, const scanned_device_t& device, bool excluded){
	LOGAssetFilteringDebug("FilterId=%u %s device with mac: %02X:%02X:%02X:%02X:%02X:%02X",
			filter.runtimedata()->filterId,
			excluded ? "excluded" : "accepted",
			device.address[5],
			device.address[4],
			device.address[3],
			device.address[2],
			device.address[1],
			device.address[0]);
}

cs_ret_code_t AssetFiltering::init() {
	// Handle multiple calls to init.
	switch (_initState) {
		case AssetFilteringState::NONE: {
			break;
		}
		case AssetFilteringState::INIT_SUCCESS: {
			LOGw("Init was already called.");
			return ERR_SUCCESS;
		}
		default: {
			LOGe("Init was already called: state=%u", _initState);
			return ERR_WRONG_STATE;
		}
	}

	// Keep up init state.
	auto retCode = initInternal();
	if (retCode == ERR_SUCCESS) {
		_initState = AssetFilteringState::INIT_SUCCESS;
	}
	else {
		_initState = AssetFilteringState::INIT_FAILED;
	}
	return retCode;
}

cs_ret_code_t AssetFiltering::initInternal() {
	LOGAssetFilteringInfo("init");

	// Can we change this to a compile time check?
	assert(AssetFilterStore::MAX_FILTER_IDS <= sizeof(filter_output_bitmasks_t::_forwardAssetId) * 8, "Too many filters for bitmask.");
	if (AssetFilterStore::MAX_FILTER_IDS > sizeof(filter_output_bitmasks_t::_forwardAssetId) * 8) {
		LOGe("Too many filters for bitmask.");
		return ERR_MISMATCH;
	}

	_filterStore = new AssetFilterStore();
	if (_filterStore == nullptr) {
		return ERR_NO_SPACE;
	}

	_filterSyncer = new AssetFilterSyncer();
	if (_filterSyncer == nullptr) {
		return ERR_NO_SPACE;
	}

	_assetForwarder = new AssetForwarder();
	if (_assetForwarder == nullptr) {
		return ERR_NO_SPACE;
	}

	_assetStore = new AssetStore();
	if (_assetStore == nullptr) {
		return ERR_NO_SPACE;
	}

#if BUILD_CLOSEST_CROWNSTONE_TRACKER == 1
	_nearestCrownstoneTracker = new NearestCrownstoneTracker();
	if (_nearestCrownstoneTracker == nullptr) {
		return ERR_NO_SPACE;
	}

	addComponent(_nearestCrownstoneTracker);
#endif

	addComponents({_filterStore, _filterSyncer, _assetForwarder, _assetStore});

	// Init components
	if (cs_ret_code_t retCode = initChildren() != ERR_SUCCESS) {
		LOGAssetFilteringWarn("init failed with code: %x", retCode);
		return retCode;
	}

	listen();
	return ERR_SUCCESS;
}


bool AssetFiltering::isInitialized() {
	return _initState == AssetFilteringState::INIT_SUCCESS;
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

void AssetFiltering::handleScannedDevice(const scanned_device_t& asset) {
	if (!_filterStore->isReady()) {
		return;
	}

	if (isAssetRejected(asset)) {
		return;
	}

	filter_output_bitmasks_t masks = getAcceptedBitmasks(asset);

	if (!masks.combined()) {
		// early return when no filter accepts the advertisement.
		return;
	}

	LOGAssetFilteringDebug("bitmask forwardSid: %x. forwardMac: %x, nearestSid: %x",
			masks._forwardAssetId,
			masks._forwardMac,
			masks._nearestAssetId);

	handleScannedDevice(masks, asset);
}


void AssetFiltering::handleScannedDevice(filter_output_bitmasks_t masks, const scanned_device_t& asset) {
	// construct short asset id
	AssetFilter sidFilter         = filterToUseForShortAssetId(masks);
	short_asset_id_t shortAssetId = filterOutputResultShortAssetId(sidFilter, asset);
	asset_record_t* assetRecord = nullptr;

	if (_assetStore != nullptr) { // Useless nullptr check?
		_assetStore->handleAcceptedAsset(asset, shortAssetId); // Should return the record,
		assetRecord = _assetStore->getRecord(shortAssetId); // because this iterates again
	}

	// throttle if the record currently exists and requires it.
	bool throttle = assetRecord != nullptr && assetRecord->isThrottled();

	if (!throttle) {
		uint16_t throttlingCounterBumpMs = 0;

		// forward sid to mesh
		if (masks._forwardAssetId && _assetForwarder != nullptr) { // Useless nullptr check?
			throttlingCounterBumpMs += _assetForwarder->sendAssetIdToMesh(asset, shortAssetId);
			LOGd("throttling bump ms: %u", throttlingCounterBumpMs);
		}

		// forward mac to mesh
		if (masks._forwardMac && _assetForwarder != nullptr) { // Useless nullptr check?
			throttlingCounterBumpMs += _assetForwarder->sendAssetMacToMesh(asset);
			LOGd("throttling bump ms: %u", throttlingCounterBumpMs);
		}

#if BUILD_CLOSEST_CROWNSTONE_TRACKER == 1
		// nearest algorithm
		if (masks._nearestAssetId && _nearestCrownstoneTracker != nullptr) { // Useless nullptr check?
			throttlingCounterBumpMs += _nearestCrownstoneTracker->handleAcceptedAsset(asset, shortAssetId);
			LOGd("throttling bump ms: %u", throttlingCounterBumpMs);
		}
#endif

		// update the throttling counter
		if (_assetStore != nullptr && assetRecord != nullptr) { // Useless nullptr check?
			LOGd("throttling bump ms: %u", throttlingCounterBumpMs);
			_assetStore->addThrottlingBump(*assetRecord, throttlingCounterBumpMs);
		}
	}
	else {
		LOGAssetFilteringInfo("throttled incoming asset advertisement");
		if (assetRecord != nullptr) {
			LOGAssetFilteringDebug("counter: %u", assetRecord->throttlingCountdownTicks);
		}
	}

	// send general asset accepted event to the firmware
	if (masks.combined()) {
		// Note: anything handling this event is unthrottled by design:
		// local firmware can keep up to date asap without penalty.
		LOGAssetFilteringDebug("Dispatch EVT_ASSET_ACCEPTED");

		AssetAcceptedEvent evtData(_filterStore->getFilter(masks.primaryFilter()), asset, masks.combined());

		event_t assetEvent(CS_TYPE::EVT_ASSET_ACCEPTED, &evtData, sizeof(evtData));
		assetEvent.dispatch();
	}
}


AssetFiltering::filter_output_bitmasks_t AssetFiltering::getAcceptedBitmasks(const scanned_device_t& device) {
	filter_output_bitmasks_t masks = {};

	for (uint8_t i = 0; i < _filterStore->getFilterCount(); ++i) {
		auto filter = AssetFilter (_filterStore->getFilter(i));

		if (filter.filterdata().metadata().flags()->flags.exclude == false) {
			if (filterAcceptsScannedDevice(filter, device)) {
				LOGAssetFilteringDebug("Accepted filterAcceptsScannedDevice");

				// update the relevant bitmask
				switch (*filter.filterdata().metadata().outputType().outFormat()) {
					case AssetFilterOutputFormat::MacOverMesh: {
						BLEutil::setBit(masks._forwardMac, i);
						break;
					}

					case AssetFilterOutputFormat::ShortAssetIdOverMesh: {
						BLEutil::setBit(masks._forwardAssetId, i);
						break;
					}

					case AssetFilterOutputFormat::ShortAssetId: {
						BLEutil::setBit(masks._nearestAssetId, i);
						break;
					}
				}
			}
		}
	}

	return masks;
}

AssetFilter AssetFiltering::filterToUseForShortAssetId(const filter_output_bitmasks_t& masks) {
	if (masks.assetId()) {
		return AssetFilter(_filterStore->getFilter(masks.primaryAssetIdFilter()));
	}
	if (masks.combined()) {
		return AssetFilter(_filterStore->getFilter(masks.primaryFilter()));
	}

	return AssetFilter(nullptr);
}

bool AssetFiltering::isAssetRejected(const scanned_device_t& device) {
	// Rejection check: looping over exclusion filters.
	for (uint8_t i = 0; i < _filterStore->getFilterCount(); ++i) {
		auto filter = AssetFilter(_filterStore->getFilter(i));

		if (filter.filterdata().metadata().flags()->flags.exclude == true) {
			if (filterAcceptsScannedDevice(filter, device)) {
				// Reject by early return.
				LogAcceptedDevice(filter, device, true);
				return true;
			}
		}
	}

	return false;
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

bool AssetFiltering::filterAcceptsScannedDevice(AssetFilter assetFilter, const scanned_device_t& asset) {
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
