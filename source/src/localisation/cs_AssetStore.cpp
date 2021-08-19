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


cs_ret_code_t AssetStore::init() {
	resetReports();
	return ERR_SUCCESS;
}

void AssetStore::handleEvent(event_t& evt) {
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
		rec.assetId = id;
		rec.closestStoneId = 0;

		// set rssi's unreasonably low so that they will be overwritten on the first observation
		rec.myRssi = compressRssi(-127, 0);
		rec.closestRssi = compressRssi(-127, 0);

		_assetRecordCount += 1;
		return &rec;
	} else {
		LogAssetStoreVerbose("can't create new asset record, maximum reached. ID: 0x%x 0x%x 0x%x",
							id.data[0], id.data[1], id.data[2] );
	}

	return nullptr;
}

void AssetStore::logRecord(asset_record_t& record) {
	LogAssetStoreVerbose(
			"ID(%x %x %x) winner(#%u, %d dB ch%u [%u]) me(%d dB ch %u)",
			record.assetId.data[0],
			record.assetId.data[1],
			record.assetId.data[2],
			record.closestStoneId,
			getRssi(record.closestRssi),
			getChannel(record.closestRssi),
			record.closestRssi,
			getRssi(record.myRssi),
			getChannel(record.myRssi));
}
