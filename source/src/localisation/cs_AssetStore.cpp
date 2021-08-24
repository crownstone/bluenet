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
#define LOGAssetStoreDebug LOGd
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
	resetReports();
	listen();

	return ERR_SUCCESS;
}

void AssetStore::handleEvent(event_t& evt) {
	if (updateLastReceivedCounterRoutine.handleEvent(evt) && updateLastSentCounterRoutine.handleEvent(evt)) {
		// short circuit: coroutines return true when they handle a EVT_TICK event.
		return;
	}

	switch (evt.type) {
		case CS_TYPE::EVT_FILTERS_UPDATED: {
			resetReports();
			break;
		}
		default: {
			break;
		}
	}
}

void AssetStore::handleAcceptedAsset(const scanned_device_t& asset, const short_asset_id_t& assetId) {
	if(auto rec = getOrCreateRecord(assetId)) {
		rec->myRssi = compressRssi(asset.rssi,asset.channel);
		rec->lastReceivedCounter = 0;
	}

	// asset store is full. Warning logged in the getOrCreate method.
}

void AssetStore::resetReports() {
	for (auto& rec : _assetRecords){
		rec = asset_record_t::clear();
	}
}

asset_record_t* AssetStore::getRecord(const short_asset_id_t& id) {
	// linear search
	for (uint8_t i = 0; i < _assetRecordCount; i++) {
		auto& rec = _assetRecords[i];
		if (rec.isValid() && rec.assetId == id) {
			return &rec;
		}
	}
	return nullptr;
}

asset_record_t* AssetStore::getOrCreateRecord(const short_asset_id_t& id) {
	if(auto rec = getRecord(id)) {
		return rec;
	}

	// not found. create new report if there is space available
	if (_assetRecordCount < MAX_REPORTS) {
		LOGAssetStoreVerbose("creating new report record");
		auto& rec = _assetRecords[_assetRecordCount];
		rec = asset_record_t::empty();
		rec.assetId = id;

		_assetRecordCount += 1;
		return &rec;
	} else {
		LOGAssetStoreVerbose("can't create new asset record, maximum reached. ID: 0x%x 0x%x 0x%x",
							id.data[0], id.data[1], id.data[2] );
	}

	return nullptr;
}

void AssetStore::addThrottlingBump(asset_record_t& record, uint16_t timeToNextThrottleOpenMs) {
	uint16_t ticks_rounded_up = (timeToNextThrottleOpenMs + THROTTLE_COUNTER_PERIOD_MS - 1)/ THROTTLE_COUNTER_PERIOD_MS;
	uint16_t total_ticks = record.throttlingCountdownTicks + ticks_rounded_up;

	LOGAssetStoreDebug("adding throttle ticks: %u for %u ms", total_ticks, timeToNextThrottleOpenMs);

	if(total_ticks > 0xff) {
		record.throttlingCountdownTicks = 0xff;
	} else {
		record.throttlingCountdownTicks = total_ticks;
	}
}


void AssetStore::incrementLastReceivedCounters() {
	for (auto& rec: _assetRecords) {
		if(!rec.isValid()) {
			// skip invalid records
			continue;
		}

		if (rec.lastReceivedCounter < 0xff) {
			rec.lastReceivedCounter++;
		}

		if(rec.lastReceivedCounter > LAST_RECEIVED_TIMEOUT_THRESHOLD) {
			LOGAssetStoreDebug("Asset timed out. %x:%x:%x",
					rec.assetId.data[0], rec.assetId.data[1], rec.assetId.data[2]);
			rec = asset_record_t::clear();
		}
	}
}

void AssetStore::decrementThrottlingCounters() {
	for (auto& rec: _assetRecords) {
		if(!rec.isValid()) {
			// skip invalid records
			continue;
		}

		if (rec.throttlingCountdownTicks > 0) {
			rec.throttlingCountdownTicks--;
		}
	}
}
