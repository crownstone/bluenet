/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 17, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <localisation/cs_AssetStore.h>
#include <logging/cs_Logger.h>
#include <protocol/cs_RssiAndChannel.h>

#define LOGAssetStoreWarn    LOGw
#define LOGAssetStoreInfo    LOGi
#define LOGAssetStoreDebug   LOGvv
#define LOGAssetStoreVerbose LOGvv

AssetStore::AssetStore()
	: updateLastReceivedCounterRoutine([this]() {
		incrementLastReceivedCounters();
		return Coroutine::delayMs(LAST_RECEIVED_COUNTER_PERIOD_MS);
	})
	, updateLastSentCounterRoutine([this]() {
		decrementThrottlingCounters();
		return Coroutine::delayMs(THROTTLE_COUNTER_PERIOD_MS);
	})

{}

cs_ret_code_t AssetStore::init() {
	LOGAssetStoreInfo("Init: using buffer of %u B", sizeof(_assetRecords));
	resetRecords();
	listen();

	return ERR_SUCCESS;
}

void AssetStore::handleEvent(event_t& event) {
	updateLastReceivedCounterRoutine.handleEvent(event);
	updateLastSentCounterRoutine.handleEvent(event);

	switch (event.type) {
		case CS_TYPE::EVT_FILTERS_UPDATED: {
			resetRecords();
			break;
		}
		default: {
			break;
		}
	}
}

void AssetStore::handleAcceptedAsset(const scanned_device_t& asset, const short_asset_id_t& assetId) {
	LOGAssetStoreVerbose("handleAcceptedAsset id=%02X:%02X:%02X", assetId.data[0], assetId.data[1], assetId.data[2]);
	auto record = getOrCreateRecord(assetId);
	if (record != nullptr) {
		record->myRssi = rssi_and_channel_t(asset.rssi, asset.channel);
		record->lastReceivedCounter = 0;
	}
	else {
		LOGAssetStoreDebug("Could not create a record for id=%02X:%02X:%02X", assetId.data[0], assetId.data[1], assetId.data[2]);
	}
}

void AssetStore::resetRecords() {
	LOGAssetStoreDebug("resetRecords");
	for (auto& record : _assetRecords){
		record.invalidate();
	}
}

asset_record_t* AssetStore::getRecord(const short_asset_id_t& id) {
	for (uint8_t i = 0; i < _assetRecordCount; ++i) {
		auto& record = _assetRecords[i];
		if (record.isValid() && record.assetId == id) {
			return &record;
		}
	}
	return nullptr;
}

asset_record_t* AssetStore::getOrCreateRecord(const short_asset_id_t& id) {
	LOGAssetStoreVerbose("getOrCreateRecord id=%02X:%02X:%02X", id.data[0], id.data[1], id.data[2]);
	uint8_t emptyIndex = 0xFF;
	uint8_t oldestIndex = 0;
	uint8_t highestCounter = 0;
	for (uint8_t i = 0; i < _assetRecordCount; ++i) {
		auto& record = _assetRecords[i];
		if (!record.isValid()) {
			emptyIndex = i;
			LOGAssetStoreVerbose("Found empty spot at index=%u", i);
		}
		else if (record.assetId == id) {
			LOGAssetStoreVerbose("Found asset at index=%u", i);
			return &record;
		}
		else if (record.lastReceivedCounter > highestCounter) {
			highestCounter = record.lastReceivedCounter;
			oldestIndex = i;
		}
	}
	// Record with given asset ID does not exist yet, create a new one.

	// First option, use empty spot.
	if (emptyIndex != 0xFF) {
		LOGAssetStoreVerbose("Creating new report record on empty spot, index=%u", emptyIndex);
		auto& record =_assetRecords[emptyIndex];
		record.empty();
		record.assetId = id;
		return &record;
	}

	// Second option, increase number of records.
	if (_assetRecordCount < MAX_RECORDS) {
		LOGAssetStoreVerbose("Add new report record, index=%u", _assetRecordCount);
		auto& record = _assetRecords[_assetRecordCount];
		record.empty();
		record.assetId = id;
		_assetRecordCount++;
		return &record;
	}

	// Last option, overwrite oldest record.
	LOGAssetStoreVerbose("Overwriting oldest record asset id=%02X:%02X:%02X, index=%u",
			_assetRecords[oldestIndex].assetId.data[0], _assetRecords[oldestIndex].assetId.data[1], _assetRecords[oldestIndex].assetId.data[2], oldestIndex);
	auto& record =_assetRecords[oldestIndex];
	record.empty();
	record.assetId = id;
	return &record;
}

// REVIEW: why add instead of set?
void AssetStore::addThrottlingBump(asset_record_t& record, uint16_t timeToNextThrottleOpenMs) {
	// REVIEW: this isn't rounded up, it gives 1 tick for 1 ms.
	uint16_t ticksRoundedUp = (timeToNextThrottleOpenMs + THROTTLE_COUNTER_PERIOD_MS - 1) / THROTTLE_COUNTER_PERIOD_MS;
	uint16_t ticksTotal = record.throttlingCountdown + ticksRoundedUp;

	LOGAssetStoreVerbose("Adding throttle ticks: %u for %u ms", ticksTotal, timeToNextThrottleOpenMs);

	if (ticksTotal > 0xFF) {
		record.throttlingCountdown = 0xFF;
	}
	else {
		record.throttlingCountdown = ticksTotal;
	}
}


void AssetStore::incrementLastReceivedCounters() {
	for (auto& record: _assetRecords) {
		if (!record.isValid()) {
			// skip invalid records
			continue;
		}

		if (record.lastReceivedCounter < 0xFF) {
			record.lastReceivedCounter++;
		}

		if (record.lastReceivedCounter >= LAST_RECEIVED_TIMEOUT_THRESHOLD_S) {
			LOGAssetStoreDebug("Asset timed out. %02X:%02X:%02X",
					record.assetId.data[0], record.assetId.data[1], record.assetId.data[2]);
			record.invalidate();
		}
	}
}

void AssetStore::decrementThrottlingCounters() {
	for (auto& record: _assetRecords) {
		if (!record.isValid()) {
			// skip invalid records
			continue;
		}

		if (record.throttlingCountdown > 0) {
			record.throttlingCountdown--;
		}
	}
}
