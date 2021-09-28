/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 7, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <localisation/cs_AssetFiltering.h>
#include <util/cs_Utils.h>

#define LOGAssetFilteringWarn LOGw
#define LOGAssetFilteringInfo LOGd
#define LOGAssetFilteringDebug LOGvv
#define LOGAssetFilteringVerbose LOGvv
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

// -------------------- init -----------------------

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

// -------------------- event handling -----------------------

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


	filter_output_bitmasks_t masks = {};

	for (uint8_t filterIndex = 0; filterIndex < _filterStore->getFilterCount(); ++filterIndex) {
		handleAcceptFilter(filterIndex, asset, masks);
	}

	if (!masks.combined()) {
		// early return when no filter accepts the advertisement.
		return;
	}

	LOGAssetFilteringDebug("bitmask forwardSid: %x. forwardMac: %x, nearestSid: %x",
			masks._forwardAssetId,
			masks._forwardMac,
			masks._nearestAssetId);

	uint8_t combinedMasks = masks.combined();

	for (uint8_t filterIndex = 0; filterIndex < _filterStore->getFilterCount(); ++filterIndex) {
		if(BLEutil::isBitSet(combinedMasks, filterIndex)) {
			auto filter = AssetFilter (_filterStore->getFilter(filterIndex));

			AssetAcceptedEvent evtData(filter, asset, combinedMasks);
			event_t assetEvent(CS_TYPE::EVT_ASSET_ACCEPTED, &evtData, sizeof(evtData));

			assetEvent.dispatch();
		}
	}
}

bool AssetFiltering::handleAcceptFilter(
		uint8_t filterIndex, const scanned_device_t& device, filter_output_bitmasks_t& masks) {
	auto filter = AssetFilter(_filterStore->getFilter(filterIndex));

	if (filter.filterdata().metadata().flags()->flags.exclude) {
		return false;
	}

	if (filter.filterAcceptsScannedDevice(device)) {
		LOGAssetFilteringDebug("Accepted filterAcceptsScannedDevice");

		// update the relevant bitmask
		switch (*filter.filterdata().metadata().outputType().outFormat()) {
			case AssetFilterOutputFormat::Mac: {
				BLEutil::setBit(masks._forwardMac, filterIndex);
				LOGAssetFilteringDebug("Accepted MacOverMesh %u", filterIndex);
				handleAssetAcceptedMacOverMesh(filterIndex, filter, device);
				return true;
			}

			case AssetFilterOutputFormat::AssetId: {
				BLEutil::setBit(masks._forwardAssetId, filterIndex);
				LOGAssetFilteringDebug("Accepted AssetIdOverMesh %u", masks);
				handleAssetAcceptedAssetIdOverMesh(filterIndex, filter, device);
				return true;
			}

#if BUILD_CLOSEST_CROWNSTONE_TRACKER == 1
			case AssetFilterOutputFormat::AssetIdNearest: {
				BLEutil::setBit(masks._nearestAssetId, filterIndex);
				LOGAssetFilteringDebug("Accepted NearestAssetId %u", masks);
				handleAssetAcceptedNearestAssetId(filterIndex, filter, device);
				return true;
			}
#endif
		}
	}

	return false;
}

// -------------------- filter handlers -----------------------

void AssetFiltering::handleAssetAcceptedMacOverMesh(
		uint8_t filterId, AssetFilter filter, const scanned_device_t& asset) {
	// construct short asset id
	asset_id_t assetId = filter.getAssetId(asset);
	asset_record_t* assetRecord = _assetStore->handleAcceptedAsset(asset, assetId);

	// throttle if the record currently exists and requires it.
	bool throttle = (assetRecord != nullptr) && (assetRecord->isThrottled());

	if (!throttle) {
		uint16_t throttlingCounterBumpMs = 0;
		throttlingCounterBumpMs += _assetForwarder->sendAssetMacToMesh(asset);
		_assetStore->addThrottlingBump(*assetRecord, throttlingCounterBumpMs);

		LOGAssetFilteringVerbose("throttling bump ms: %u", throttlingCounterBumpMs);
	}
	else {
		LOGAssetFilteringVerbose("Throttled asset id=%02X:%02X:%02X counter=%u",
				assetId.data[0],
				assetId.data[1],
				assetId.data[2],
				assetRecord->throttlingCountdown);
	}

	// TODO: send uart update
}

void AssetFiltering::handleAssetAcceptedAssetIdOverMesh(
		uint8_t filterId, AssetFilter filter, const scanned_device_t& asset) {

	asset_id_t assetId = filter.getAssetId(asset);
	asset_record_t* assetRecord = _assetStore->handleAcceptedAsset(asset, assetId);

	// throttle if the record currently exists and requires it.
	bool throttle = (assetRecord != nullptr) && (assetRecord->isThrottled());

	if (!throttle) {
		uint16_t throttlingCounterBumpMs = 0;
		uint8_t filterBitmask = 0;
		BLEutil::setBit(filterBitmask, filterId);
		throttlingCounterBumpMs += _assetForwarder->sendAssetIdToMesh(asset, assetId, filterBitmask);
		_assetStore->addThrottlingBump(*assetRecord, throttlingCounterBumpMs);

		LOGAssetFilteringVerbose("throttling bump ms: %u", throttlingCounterBumpMs);
	}
	else {
		LOGAssetFilteringVerbose("Throttled asset id=%02X:%02X:%02X counter=%u",
				assetId.data[0],
				assetId.data[1],
				assetId.data[2],
				assetRecord->throttlingCountdown);
	}

	// TODO: send uart update
}


void AssetFiltering::handleAssetAcceptedNearestAssetId(
		uint8_t filterId, AssetFilter filter, const scanned_device_t& asset) {
#if BUILD_CLOSEST_CROWNSTONE_TRACKER == 1
	asset_id_t assetId = filter.getAssetId(asset);
	asset_record_t* assetRecord   = _assetStore->handleAcceptedAsset(asset, assetId);

	// throttle if the record currently exists and requires it.
	// TODO: always throttle when not nearest
	bool throttle = (assetRecord != nullptr) && (assetRecord->isThrottled());

	if (!throttle) {
		uint8_t filterBitmask = 0;
		BLEutil::setBit(filterBitmask, filterId);

		uint16_t throttlingCounterBumpMs = 0;
		throttlingCounterBumpMs += _nearestCrownstoneTracker->handleAcceptedAsset(asset, assetId, filterBitmask);
		_assetStore->addThrottlingBump(*assetRecord, throttlingCounterBumpMs);

		LOGAssetFilteringVerbose("throttling bump ms: %u", throttlingCounterBumpMs);
	}
	else {
		LOGAssetFilteringVerbose("Throttled asset id=%02X:%02X:%02X counter=%u",
				assetId.data[0],
				assetId.data[1],
				assetId.data[2],
				assetRecord->throttlingCountdown);
	}
#endif
}

// ---------------------------- utils ----------------------------


bool AssetFiltering::isAssetRejected(const scanned_device_t& device) {
	// Rejection check: looping over exclusion filters.
	for (uint8_t i = 0; i < _filterStore->getFilterCount(); ++i) {
		auto filter = AssetFilter(_filterStore->getFilter(i));

		if (filter.filterdata().metadata().flags()->flags.exclude == true) {
			if (filter.filterAcceptsScannedDevice(device)) {
				// Reject by early return.
				LogAcceptedDevice(filter, device, true);
				return true;
			}
		}
	}

	return false;
}
