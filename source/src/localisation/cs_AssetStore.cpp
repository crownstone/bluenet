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
	LOGAssetStoreInfo("Init: using buffer of %u B", sizeof(_store));
	_store.clear();
	listen();

	return ERR_SUCCESS;
}

void AssetStore::handleEvent(event_t& event) {
	updateLastReceivedCounterRoutine.handleEvent(event);
	updateLastSentCounterRoutine.handleEvent(event);

	switch (event.type) {
		case CS_TYPE::EVT_FILTERS_UPDATED: {
			LOGAssetStoreDebug("resetRecords");
			_store.clear();
			break;
		}
		default: {
			break;
		}
	}
}

asset_record_t* AssetStore::handleAcceptedAsset(const scanned_device_t& asset, const asset_id_t& assetId) {
	LOGAssetStoreVerbose("handleAcceptedAsset id=%02X:%02X:%02X", assetId.data[0], assetId.data[1], assetId.data[2]);
	asset_record_t* record = getOrCreateRecord(assetId);
	if (record != nullptr) {
		record->myRssi = rssi_and_channel_t(asset.rssi, asset.channel);
		record->lastReceivedCounter = 0;
	}
	else {
		LOGAssetStoreDebug("Could not create a record for id=%02X:%02X:%02X", assetId.data[0], assetId.data[1], assetId.data[2]);
	}
	return record;
}

asset_record_t* AssetStore::getRecord(const asset_id_t& id) {
	return _store.get(id);
}

asset_record_t* AssetStore::getOrCreateRecord(const asset_id_t& id) {
	LOGAssetStoreVerbose("getOrCreateRecord id=%02X:%02X:%02X", id.data[0], id.data[1], id.data[2]);

	if(asset_record_t* rec = _store.getOrAdd(id)) {
		// record found, or empty space was newly occupied.
		rec->empty();
		rec->assetId = id;
		return rec;
	}

	// Last option, overwrite oldest record.
	asset_record_t* oldestRecord = _store.begin();
	for (asset_record_t* record = _store.begin(); record != _store.end(); record++) {
		if (record->lastReceivedCounter > oldestRecord->lastReceivedCounter) {
			oldestRecord = record;
		}
	}

	LOGAssetStoreVerbose(
			"Overwriting oldest record asset id=%02X:%02X:%02X",
			oldestRecord->assetId.data[0],
			oldestRecord->assetId.data[1],
			oldestRecord->assetId.data[2]);

	oldestRecord->empty();
	oldestRecord->assetId = id;
	return oldestRecord;
}

void AssetStore::addThrottlingBump(asset_record_t& record, uint16_t timeToNextThrottleOpenMs) {

	LOGAssetStoreVerbose("Adding throttle ticks: %u for %u ms", throttlingBumpMsToTicks(timeToNextThrottleOpenMs), timeToNextThrottleOpenMs);

	record.addThrottlingCountdown(throttlingBumpMsToTicks(timeToNextThrottleOpenMs));
}

uint16_t AssetStore::throttlingBumpMsToTicks(uint16_t timeToNextThrottleOpenMs) {
	// 'Ceil division' equal to:
	//         timeToNextThrottleOpenMs / THROTTLE_COUNTER_PERIOD_MS if the division is exact
	//     1 + timeToNextThrottleOpenMs / THROTTLE_COUNTER_PERIOD_MS if there is a remainder.
	uint16_t ticksRoundedUp = (timeToNextThrottleOpenMs + THROTTLE_COUNTER_PERIOD_MS - 1) / THROTTLE_COUNTER_PERIOD_MS;
	return ticksRoundedUp;
}


void AssetStore::incrementLastReceivedCounters() {
	for (auto& record: _store) {
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
	for (auto& record: _store) {
		if (!record.isValid()) {
			// skip invalid records
			continue;
		}

		if (record.throttlingCountdown > 0) {
			record.throttlingCountdown--;
		}
	}
}
