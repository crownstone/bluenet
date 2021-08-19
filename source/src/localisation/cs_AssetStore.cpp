/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 17, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <localisation/cs_AssetStore.h>
#include <logging/cs_Logger.h>
#include <util/cs_Rssi.h>

#define LogAssetStoreWarn LOGw
#define LogAssetStoreDebug LOGd
#define LogAssetStoreVerbose LOGv


AssetStore::AssetStore() : counterUpdateRoutine([this]() {
	incrementRecordCounters();
	return Coroutine::delayMs(COUNTER_UPDATE_PERIOD_MS);
}) {

}

cs_ret_code_t AssetStore::init() {
	resetReports();
	listen();

	return ERR_SUCCESS;
}

void AssetStore::handleEvent(event_t& evt) {
	if (counterUpdateRoutine.handleEvent(evt)) {
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

void AssetStore::resetReports() {
	for (auto& rec : _assetRecords){
		rec = asset_record_t::empty();
	}
}

asset_record_t* AssetStore::getOrCreateRecord(short_asset_id_t& id) {
	// linear search
	for (uint8_t i = 0; i < _assetRecordCount; i++) {
		auto& rec = _assetRecords[i];
		if (rec.assetId == id) {
			return &rec;
		}
	}

	// not found. create new report if there is space available
	if (_assetRecordCount < MAX_REPORTS) {
		LogAssetStoreVerbose("creating new report record");
		auto& rec = _assetRecords[_assetRecordCount];
		rec = asset_record_t::empty();
		rec.assetId = id;

		_assetRecordCount += 1;
		return &rec;
	} else {
		LogAssetStoreVerbose("can't create new asset record, maximum reached. ID: 0x%x 0x%x 0x%x",
							id.data[0], id.data[1], id.data[2] );
	}

	return nullptr;
}


void AssetStore::incrementRecordCounters() {
	for (auto& rec: _assetRecords) {
		if (rec.lastReceivedCounter < 0xff) {
			rec.lastReceivedCounter++;
		}
		if (rec.lastSentCounter < 0xff) {
			rec.lastSentCounter++;
		}
	}
}
