/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 10, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <protocol/cs_Typedefs.h>
#include <localisation/cs_AssetFilterStore.h>
#include <events/cs_EventListener.h>

/**
 * Class that takes care of synchronizing the asset filters between crownstones.
 *
 * - Regularly informs other crownstones of the master version and CRC.
 * - Will connect and update the asset filters of a crownstone with older master version.
 */
class AssetFilterSyncer: EventListener {
public:
	constexpr static uint16_t VERSION_BROADCAST_INTERVAL_SECONDS = 5 * 60;

	cs_ret_code_t init(AssetFilterStore& store);

	/**
	 * Sends the master version and CRC over the mesh and UART.
	 */
	void sendVersion();


private:
	enum class SyncStep {
		NONE,
		CONNECT,
		GET_FILTER_SUMMARIES,
		REMOVE_FILTERS,
		UPLOAD_FILTERS,
		COMMIT,
		DISCONNECT
	};

	AssetFilterStore* _store = nullptr;

	SyncStep _step = SyncStep::NONE;
	uint8_t _nextFilterIndex;
	uint16_t _nextChunkIndex;
	uint8_t _filterUploadCount;
	uint8_t _filterIdsToUpload[AssetFilterStore::MAX_FILTER_IDS];
	uint8_t _filterRemoveCount;
	uint8_t _filterIdsToRemove[AssetFilterStore::MAX_FILTER_IDS];


	void setStep(SyncStep step);
	void reset();
	bool shouldSync(asset_filter_cmd_protocol_t protocol, uint16_t masterVersion, uint16_t masterCrc);

	void syncFilters(stone_id_t stoneId);
	void connect(stone_id_t stoneId);
	void removeNextFilter();
	void uploadNextFilter();
	void commit();
	void disconnect();


	cs_ret_code_t onVersion(stone_id_t stoneId, cs_mesh_model_msg_asset_filter_version_t& packet);

	void onConnectResult(cs_ret_code_t retCode);
	void onDisconnect();
	void onWriteResult(cs_central_write_result_t& result);
	void onFilterSummaries(cs_data_t& payload);
	void onTick(uint32_t tickCount);
public:
	/**
	 * Internal usage.
	 */
	void handleEvent(event_t& evt);
};


