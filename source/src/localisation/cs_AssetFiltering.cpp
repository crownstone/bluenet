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
#define LOGAssetFilteringDebug LOGvv
#define LOGAssetFilteringVerbose LOGvv
#define LogLevelAssetFilteringDebug SERIAL_VERY_VERBOSE
#define LogLevelAssetFilteringVerbose SERIAL_VERY_VERBOSE

void LogAcceptedDevice(AssetFilter filter, const scanned_device_t& device, bool excluded) {
	LOGAssetFilteringDebug(
			"FilterId=%u passed device with mac: %02X:%02X:%02X:%02X:%02X:%02X exclude=%u",
			filter.runtimedata()->filterId,
			device.address[5],
			device.address[4],
			device.address[3],
			device.address[2],
			device.address[1],
			device.address[0],
			excluded);
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
#endif

	// Init components
	if (cs_ret_code_t retCode = initChildren() != ERR_SUCCESS) {
		LOGAssetFilteringWarn("init failed with code: %x", retCode);
		return retCode;
	}

	// TODO: move these constants or tie the forwarder up with the store so that they
	// can decide how fast to tick and trottle
	_assetForwarder->setThrottleCountdownBumpTicks(
			_assetStore->throttlingBumpMsToTicks(_assetForwarder->MIN_THROTTLED_ADVERTISEMENT_PERIOD_MS));

	listen();
	return ERR_SUCCESS;
}

std::vector<Component*> AssetFiltering::getChildren() {
	return {
		_filterStore, _filterSyncer, _assetForwarder, _assetStore,
#if BUILD_CLOSEST_CROWNSTONE_TRACKER == 1
				_nearestCrownstoneTracker,
#endif
	};
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
		default: break;
	}
}

void AssetFiltering::handleScannedDevice(const scanned_device_t& asset) {
	if (!_filterStore->isReady()) {
		return;
	}

	LOGAssetFilteringVerbose(
			"Scanned device mac=%02X:%02X:%02X:%02X:%02X:%02X",
			asset.address[5],
			asset.address[4],
			asset.address[3],
			asset.address[2],
			asset.address[1],
			asset.address[0]);
	_logArray(LogLevelAssetFilteringVerbose, true, asset.data, asset.dataSize);

	if (isAssetRejected(asset)) {
		return;
	}

	for (uint8_t filterIndex = 0; filterIndex < _filterStore->getFilterCount(); ++filterIndex) {
		checkIfFilterAccepts(filterIndex, asset);
	}

	_assetForwarder->flush();
}

bool AssetFiltering::checkIfFilterAccepts(uint8_t filterIndex, const scanned_device_t& device) {
	auto filter = AssetFilter(_filterStore->getFilter(filterIndex));

	if (filter.filterdata().metadata().flags()->flags.exclude) {
		return false;
	}

	if (filter.filterAcceptsScannedDevice(device)) {
		handleAcceptedAsset(filterIndex, filter, device);

		AssetAcceptedEvent evtData(filterIndex, filter, device);
		event_t assetEvent(CS_TYPE::EVT_ASSET_ACCEPTED, &evtData, sizeof(evtData));
		assetEvent.dispatch();

		return true;
	}

	return false;
}

void AssetFiltering::handleAcceptedAsset(uint8_t filterIndex, AssetFilter filter, const scanned_device_t& asset) {
	switch (*filter.filterdata().metadata().outputType().outFormat()) {
		case AssetFilterOutputFormat::Mac: {
			LOGAssetFilteringDebug("FilterId %u accepted OutputFormat::Mac", filterIndex);
			handleAcceptedAssetOutputMac(filterIndex, filter, asset);
			return;
		}

		case AssetFilterOutputFormat::AssetId: {
			LOGAssetFilteringDebug("FilterId %u accepted OutputFormat::AssetId", filterIndex);
			handleAcceptedAssetOutputAssetId(filterIndex, filter, asset);
			return;
		}

		case AssetFilterOutputFormat::None: {
			LOGAssetFilteringDebug("FilterId %u Accepted OutputFormat::None", filterIndex);
			return;
		}

#if BUILD_CLOSEST_CROWNSTONE_TRACKER == 1
		case AssetFilterOutputFormat::AssetIdNearest: {
			LOGAssetFilteringDebug("FilterId %u accepted OutputFormat::AssetIdNearest", filterIndex);
			handleAcceptedAssetOutputAssetIdNearest(filterIndex, filter, asset);
			return;
		}
#endif
	}
}

// -------------------- filter handlers -----------------------

void AssetFiltering::handleAcceptedAssetOutputMac(uint8_t filterId, AssetFilter filter, const scanned_device_t& asset) {
	// construct short asset id
	asset_id_t assetId          = filter.getAssetId(asset);
	asset_record_t* assetRecord = _assetStore->handleAcceptedAsset(asset, assetId);

	// throttle if the record currently exists and requires it.
	bool throttle               = (assetRecord != nullptr) && (assetRecord->isThrottled());

	if (!throttle) {
		_assetForwarder->sendAssetMacToMesh(assetRecord, asset);
	}
	else {
		LOGAssetFilteringVerbose(
				"Throttled asset id=%02X:%02X:%02X counter=%u",
				assetId.data[0],
				assetId.data[1],
				assetId.data[2],
				assetRecord->throttlingCountdown);
	}
}

void AssetFiltering::handleAcceptedAssetOutputAssetId(
		uint8_t filterId, AssetFilter filter, const scanned_device_t& asset) {

	asset_id_t assetId          = filter.getAssetId(asset);
	asset_record_t* assetRecord = _assetStore->handleAcceptedAsset(asset, assetId);

	// throttle if the record currently exists and requires it.
	bool throttle               = (assetRecord != nullptr) && (assetRecord->isThrottled());

	if (!throttle) {
		uint8_t filterBitmask = 0;
		CsUtils::setBit(filterBitmask, filterId);
		_assetForwarder->sendAssetIdToMesh(assetRecord, asset, assetId, filterBitmask);
	}
	else {
		LOGAssetFilteringVerbose(
				"Throttled asset id=%02X:%02X:%02X counter=%u",
				assetId.data[0],
				assetId.data[1],
				assetId.data[2],
				assetRecord->throttlingCountdown);
	}
}

void AssetFiltering::handleAcceptedAssetOutputAssetIdNearest(
		uint8_t filterId, AssetFilter filter, const scanned_device_t& asset) {
#if BUILD_CLOSEST_CROWNSTONE_TRACKER == 1
	asset_id_t assetId          = filter.getAssetId(asset);
	asset_record_t* assetRecord = _assetStore->handleAcceptedAsset(asset, assetId);

	// throttle if the record currently exists and requires it.
	bool throttle               = (assetRecord != nullptr) && (assetRecord->isThrottled());

	if (!throttle) {
		uint8_t filterBitmask = 0;
		CsUtils::setBit(filterBitmask, filterId);

		// if nearest wants to send a message, do so.
		// (nearest algorithm only wants to send messages when we are nearest)
		if (_nearestCrownstoneTracker->handleAcceptedAsset(asset, assetId, filterBitmask)) {
			_assetForwarder->sendAssetIdToMesh(assetRecord, asset, assetId, filterBitmask);
		}
	}
	else {
		LOGAssetFilteringVerbose(
				"Throttled asset id=%02X:%02X:%02X counter=%u",
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
