/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 17, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <events/cs_EventListener.h>.h>
#include <localisation/cs_AssetRecord.h>

class AssetStore  : EventListener {
private:
	static constexpr auto MAX_REPORTS = 10u;

public:
	void init();

	void handleEvent(event_t& evt) override;

	asset_record_t* getRecord(short_asset_id_t sid);

	/**
	 * returns a pointer to the found record, possibly empty.
	 * returns nullptr if not found and creating a new one was
	 * impossible.
	 */
	asset_record_t* getOrCreateRecord(short_asset_id_t& id);


private:
	/**
	 * Relevant data for this algorithm per asset_id.
	 * Possible strategies to reduce memory:
	 *  - when full, remove worst personal_rssi.
	 *  - only store if observed the asset personally.
	 *  - ...
	 *
	 *  The array is 'front loaded'. entries with index < _assetRecordCount
	 *  are valid, other entries are not.
	 */
	asset_record_t _assetRecords[MAX_REPORTS];

	/**
	 * Current number of valid records in the _assetRecords array.
	 */
	uint8_t _assetRecordCount = 0;

	/**
	 * Assumes my_id is set to the stone id of this crownstone.
	 * Sets the reporter id of the personal report to my_id.
	 * Sets the reporter id of the winning report to 0.
	 * Sets the rssi value of both these reports to -127.
	 */
	void resetReports();



	void logRecord(asset_record_t& record);

};
