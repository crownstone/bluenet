/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 10, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <common/cs_Component.h>
#include <events/cs_EventListener.h>

#include <protocol/cs_Typedefs.h>
#include <localisation/cs_AssetFilterStore.h>

/**
 * Class that takes care of synchronizing the asset filters between crownstones.
 *
 * - Regularly informs other crownstones of the master version and CRC.
 * - Will connect and update the asset filters of a crownstone with an older master version.
 */
class AssetFilterSyncer : public EventListener, public Component {
public:
	/**
	 * Interval at which the master version is broadcasted.
	 */
	constexpr static uint16_t VERSION_BROADCAST_NORMAL_INTERVAL_SECONDS = 5 * 60;
	constexpr static uint16_t VERSION_BROADCAST_LOW_INTERVAL_SECONDS = 1;

	/**
	 * How long the low interval will be used after setting it to low interval.
	 */
	constexpr static uint16_t VERSION_BROADCAST_INTERVAL_RESET_SECONDS = 60;

	/**
	 * Init the class:
	 * - Starts listening for events.
	 */
	cs_ret_code_t init();

private:
	/**
	 * Async steps that are taken when synchronizing filters to another crownstone.
	 */
	enum class SyncStep {
		NONE,
		CONNECT,
		GET_FILTER_SUMMARIES,
		REMOVE_FILTERS,
		UPLOAD_FILTERS,
		COMMIT,
		DISCONNECT
	};

	/**
	 * Results of comparing master version with another crownstone.
	 */
	enum class VersionCompare {
		UNKOWN,
		OLDER,
		EQUAL,
		NEWER
	};

	/**
	 * Pointer to the (intialized) filter store.
	 */
	AssetFilterStore* _store = nullptr;

	/**
	 * Current step.
	 */
	SyncStep _step = SyncStep::NONE;

	/**
	 * Next index of filter IDs to upload/remove array.
	 */
	uint8_t _nextFilterIndex;

	/**
	 * Next chunk index to upload.
	 */
	uint16_t _nextChunkIndex;

	/**
	 * Filter IDs that should be uploaded.
	 */
	uint8_t _filterIdsToUpload[AssetFilterStore::MAX_FILTER_IDS];
	uint8_t _filterUploadCount;

	/**
	 * Filter IDs that should be removed.
	 */
	uint8_t _filterIdsToRemove[AssetFilterStore::MAX_FILTER_IDS];
	uint8_t _filterRemoveCount;

	/**
	 * Countdown counter that keeps track when the send version interval should go back to normal again.
	 */
	uint16_t _sendVersionAtLowIntervalCountdown = 0;

	/**
	 * Countdown counter that keeps track when to call sendVersion again.
	 */
	uint16_t _sendVersionCountdown = 0;

	/**
	 * Sends the master version and CRC over the mesh.
	 */
	void sendVersion(bool reliable);

	/**
	 * Send version at a low interval for some time.
	 */
	void sendVersionAtLowInterval();

	/**
	 * Set the current step of the sync process.
	 */
	void setStep(SyncStep step);

	/**
	 * Abort the sync process.
	 */
	void reset();

	/**
	 * Compare given master version with the master version of this crownstone.
	 */
	VersionCompare compareToMyVersion(asset_filter_cmd_protocol_t protocol, uint16_t masterVersion, uint32_t masterCrc);

	/**
	 * Start updating filters of another crownstone.
	 */
	void syncFilters(stone_id_t stoneId);

	/**
	 * Steps of the sync process.
	 */
	void connect(stone_id_t stoneId);
	void removeNextFilter();
	void uploadNextFilter();
	void commit();
	void disconnect();

	/**
	 * When the whole sync process to a stone was successful, call done().
	 */
	void done();

	/**
	 * Handle a received version mesh message.
	 */
	cs_ret_code_t onVersion(stone_id_t stoneId, cs_mesh_model_msg_asset_filter_version_t& packet);

	/**
	 * Handle a possible change of "filter modification in progress".
	 */
	void onModificationInProgress(bool inProgress);

	/**
	 * Handle steps of the sync process.
	 */
	void onConnectResult(cs_ret_code_t retCode);
	void onDisconnect();
	void onWriteResult(cs_central_write_result_t& result);
	void onFilterSummaries(cs_data_t& payload);

	/**
	 * Handle the tick event.
	 */
	void onTick(uint32_t tickCount);

public:
	/**
	 * Internal usage.
	 */
	void handleEvent(event_t& evt);
};


