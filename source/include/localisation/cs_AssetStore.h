/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 17, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <common/cs_Component.h>
#include <events/cs_EventListener.h>

#include <localisation/cs_AssetRecord.h>
#include <util/cs_Coroutine.h>
#include <util/cs_Store.h>

class AssetStore : public EventListener, public Component {
public:
	// ===================== public settings =====================

	/**
	 * Time in seconds after which a record is timed out.
	 *
	 * Must be smaller than 0xFF.
	 */
	static constexpr uint8_t LAST_RECEIVED_TIMEOUT_THRESHOLD_S = 250;

	/**
	 * Interval at which the timeout counter is increased, should be 1 second.
	 */
	static constexpr auto LAST_RECEIVED_COUNTER_PERIOD_MS = 1000;

	/**
	 * Interval at which the throttle countdown is decreased.
	 */
	static constexpr auto THROTTLE_COUNTER_PERIOD_MS = 100;

	// ===================== public methods =====================

	AssetStore();
	cs_ret_code_t init() override;

	void handleEvent(event_t& evt) override;

	/**
	 * Get or create a record for the given assetId.
	 * Then update rssi values according to the incoming scan and
	 * revert the lastReceivedCounter to 0.
	 *
	 * Returns the adjusted record if found, else returns nullptr
	 */
	asset_record_t* handleAcceptedAsset(const scanned_device_t& asset, const asset_id_t& assetId);

	/**
	 * returns a pointer of record if found,
	 * else returns nullptr.
	 */
	asset_record_t* getRecord(const asset_id_t& id);

	/**
	 * Adds a value to the records' throttlingCountdownTicks.
	 * This will ensure that the record.isThrottled() will be true
	 * for (at least) timeToNextThrottleOpenMs.
	 */
	void addThrottlingBump(asset_record_t& record, uint16_t timeToNextThrottleOpenMs);

	/**
	 * Convert ms to ticks, rounding fractional parts upwards.
	 */
	uint16_t throttlingBumpMsToTicks(uint16_t timeToNextThrottleOpenMs);

private:
	// =================== private settings ===================
	static constexpr auto MAX_RECORDS = 50u;

	// =================== private variables ===================

	Store<asset_record_t, MAX_RECORDS> _store;

	Coroutine updateLastReceivedCounterRoutine;
	Coroutine updateLastSentCounterRoutine;

	// =================== private methods ===================

	/**
	 * returns a pointer of record if found,
	 * else tries to create a new blank record and return a pointer to that,
	 * else returns nullptr.
	 */
	asset_record_t* getOrCreateRecord(const asset_id_t& id);

	/**
	 * Adds 1 to the update/sent counters of all valid records, until 0xff is reached.
	 */
	void incrementLastReceivedCounters();

	/**
	 * Decrements throttlingCountdownTicks by 1of all valid records, until zero is reached.
	 */
	void decrementThrottlingCounters();
};
