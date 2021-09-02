/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 17, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <localisation/cs_AssetStore.h>
#include <logging/cs_Logger.h>
#include <util/cs_Rssi.h>

#define LOGAssetStoreWarn LOGw
#define LOGAssetStoreDebug LOGvv
#define LOGAssetStoreVerbose LOGv

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
	auto record = getOrCreateRecord(assetId);
	if (record != nullptr) {
		record->myRssi = compressRssi(asset.rssi, asset.channel);
		record->lastReceivedCounter = 0;
	}
}

void AssetStore::resetRecords() {
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
	uint8_t emptyIndex = 0xFF;
	for (uint8_t i = 0; i < _assetRecordCount; ++i) {
		auto& record = _assetRecords[i];
		if (!record.isValid()) {
			emptyIndex = i;
		}
		else if (record.assetId == id) {
			return &record;
		}
	}
	// Record did not exist yet, create a new one.

	// First, use empty spots.
	if (emptyIndex != 0xFF) {
		LOGAssetStoreVerbose("Creating new report record on empty spot, index=%u", emptyIndex);
		auto& record =_assetRecords[emptyIndex];
		record.empty();
		record.assetId = id;
		return &record;
	}

	// Second, increase number of records.
	if (_assetRecordCount < MAX_RECORDS) {
		LOGAssetStoreVerbose("Add new report record, index=%u", _assetRecordCount);
		auto& record = _assetRecords[_assetRecordCount];
		record.empty();
		record.assetId = id;
		_assetRecordCount++;
		return &record;
	}

	// REVIEW: overwrite oldest record instead?
	LOGAssetStoreDebug("Can't create new asset record, maximum reached. ID: 0x%x 0x%x 0x%x",
						id.data[0], id.data[1], id.data[2] );
	return nullptr;
}

// REVIEW: why add instead of set?
void AssetStore::addThrottlingBump(asset_record_t& record, uint16_t timeToNextThrottleOpenMs) {
	// REVIEW: this isn't rounded up, it gives 1 tick for 1 ms.
	uint16_t ticksRoundedUp = (timeToNextThrottleOpenMs + THROTTLE_COUNTER_PERIOD_MS - 1) / THROTTLE_COUNTER_PERIOD_MS;
	uint16_t ticksTotal = record.throttlingCountdown + ticksRoundedUp;

	LOGAssetStoreDebug("adding throttle ticks: %u for %u ms", ticksTotal, timeToNextThrottleOpenMs);

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
			LOGAssetStoreDebug("Asset timed out. %x:%x:%x",
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
