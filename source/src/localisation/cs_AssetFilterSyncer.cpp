/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 10, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <localisation/cs_AssetFilterSyncer.h>
#include <protocol/cs_ErrorCodes.h>
#include <events/cs_Event.h>
#include <localisation/cs_AssetFilterStore.h>
#include <util/cs_Lollipop.h>
#include <util/cs_Math.h>

#define LOGAssetFilterSyncerInfo LOGi
#define LOGAssetFilterSyncerDebug LOGvv
#define LOGAssetFilterSyncerVerbose LOGvv

cs_ret_code_t AssetFilterSyncer::init() {
	_store = getComponent<AssetFilterStore>();

	sendVersionAtLowInterval();
	listen();

	return ERR_SUCCESS;
}

void AssetFilterSyncer::sendVersion(bool reliable) {
	if (_store == nullptr) {
		LOGe("Store is nullptr");
		return;
	}
	if (_store->isInProgress()) {
		LOGAssetFilterSyncerDebug("Store is in progress: don't send version");
		return;
	}
	uint16_t masterVersion = _store->getMasterVersion();
	uint32_t masterCrc = _store->getMasterCrc();
	LOGAssetFilterSyncerDebug("sendVersion reliable=%u version=%u crc=%u", reliable, masterVersion, masterCrc);

	cs_mesh_model_msg_asset_filter_version_t versionPacket = {
			.protocol = ASSET_FILTER_CMD_PROTOCOL_VERSION,
			.masterVersion = masterVersion,
			.masterCrc = masterCrc
	};

	TYPIFY(CMD_SEND_MESH_MSG) meshMsg;
	meshMsg.type = CS_MESH_MODEL_TYPE_ASSET_FILTER_VERSION;
	meshMsg.reliability = reliable ? CS_MESH_RELIABILITY_MEDIUM : CS_MESH_RELIABILITY_LOWEST;
	meshMsg.urgency = CS_MESH_URGENCY_LOW;
	meshMsg.flags.flags.noHops = true;
	meshMsg.payload = reinterpret_cast<uint8_t*>(&versionPacket);
	meshMsg.size = sizeof(versionPacket);

	event_t event(CS_TYPE::CMD_SEND_MESH_MSG, &meshMsg, sizeof(meshMsg));
	event.dispatch();
}

cs_ret_code_t AssetFilterSyncer::onVersion(stone_id_t stoneId, cs_mesh_model_msg_asset_filter_version_t& packet) {
	LOGAssetFilterSyncerDebug("onVersion stoneId=%u protocol=%u version=%u crc=%u",
			stoneId,
			packet.protocol,
			packet.masterVersion,
			packet.masterCrc);
	VersionCompare compare = compareToMyVersion(packet.protocol, packet.masterVersion, packet.masterCrc);
	switch (compare) {
		case VersionCompare::OLDER: {
			// The stone has an older version: sync it.
			syncFilters(stoneId);
			break;
		}
		case VersionCompare::NEWER: {
			// The stone has a newer version: inform it that we should be synced.
			// Should not be reliable, as we currently get a call to onVersion each for each repeat.
			sendVersion(false);
			break;
		}
		default: {
			break;
		}
	}
	return ERR_SUCCESS;
}

AssetFilterSyncer::VersionCompare AssetFilterSyncer::compareToMyVersion(asset_filter_cmd_protocol_t protocol, uint16_t masterVersion, uint32_t masterCrc) {
	if (protocol != ASSET_FILTER_CMD_PROTOCOL_VERSION) {
		LOGAssetFilterSyncerInfo("Unknown protocol: %u", protocol);
		return VersionCompare::UNKOWN;
	}

	uint16_t myMasterVersion = _store->getMasterVersion();
	uint32_t myMasterCrc = _store->getMasterCrc();

	if (myMasterVersion == 0) {
		if (masterVersion == 0) {
			// CRC can be ignored.
			LOGAssetFilterSyncerDebug("Both have no valid filters");
			return VersionCompare::EQUAL;
		}
		LOGAssetFilterSyncerDebug("We have no valid filters");
		return VersionCompare::NEWER;
	}

	if (masterVersion == myMasterVersion) {
		if (masterCrc == myMasterCrc) {
			LOGAssetFilterSyncerDebug("Version and CRC match");
			return VersionCompare::EQUAL;
		}
		LOGAssetFilterSyncerInfo("Equal version, but different CRC");
		if (masterCrc > myMasterCrc) {
			return VersionCompare::NEWER;
		}
		else {
			return VersionCompare::OLDER;
		}
	}

	if (Lollipop::isNewer(myMasterVersion, masterVersion, 0xFFFF)) {
		return VersionCompare::NEWER;
	}
	else {
		return VersionCompare::OLDER;
	}
}

void AssetFilterSyncer::setStep(SyncStep step) {
	LOGAssetFilterSyncerDebug("Set step %u", step);
	_step = step;
}

void AssetFilterSyncer::reset() {
	LOGAssetFilterSyncerDebug("Reset");
	if (_step > SyncStep::CONNECT && _step != SyncStep::DISCONNECT) {
		disconnect();
	}
	setStep(SyncStep::NONE);
	sendVersionAtLowInterval();
}

void AssetFilterSyncer::syncFilters(stone_id_t stoneId) {
	if (_step != SyncStep::NONE) {
		return;
	}
	if (_store->isInProgress()) {
		LOGAssetFilterSyncerDebug("Store is in progress");
		return;
	}
	LOGAssetFilterSyncerInfo("syncFilters to stoneId=%u", stoneId);
	connect(stoneId);
}

void AssetFilterSyncer::connect(stone_id_t stoneId) {
	LOGAssetFilterSyncerDebug("connect");
	TYPIFY(CMD_CS_CENTRAL_CONNECT) packet;
	packet.stoneId = stoneId;
	event_t event(CS_TYPE::CMD_CS_CENTRAL_CONNECT, &packet, sizeof(packet));
	event.dispatch();
	if (event.result.returnCode != ERR_WAIT_FOR_SUCCESS) {
		reset();
		return;
	}
	setStep(SyncStep::CONNECT);
}

void AssetFilterSyncer::removeNextFilter() {
	LOGAssetFilterSyncerDebug("removeNextFilter _nextFilterIndex=%u _filterRemoveCount=%u", _nextFilterIndex, _filterRemoveCount);
	if (_nextFilterIndex == _filterRemoveCount) {
		// We're done
		_nextFilterIndex = 0;
		uploadNextFilter();
		return;
	}

	asset_filter_cmd_remove_filter_t removeCmd = {
			.protocolVersion = ASSET_FILTER_CMD_PROTOCOL_VERSION,
			.filterId = _filterIdsToRemove[_nextFilterIndex]
	};

	TYPIFY(CMD_CS_CENTRAL_WRITE) packet;
	packet.commandType = CTRL_CMD_FILTER_REMOVE;
	packet.data = cs_data_t(reinterpret_cast<uint8_t*>(&removeCmd), sizeof(removeCmd));

	event_t event(CS_TYPE::CMD_CS_CENTRAL_WRITE, &packet, sizeof(packet));
	event.dispatch();
	if (event.result.returnCode != ERR_WAIT_FOR_SUCCESS) {
		reset();
		return;
	}

	setStep(SyncStep::REMOVE_FILTERS);
	_nextFilterIndex++;
}

void AssetFilterSyncer::uploadNextFilter() {
	LOGAssetFilterSyncerDebug("uploadNextFilter _nextFilterIndex=%u _filterUploadCount=%u _nextChunkIndex=%u",
			_nextFilterIndex,
			_filterUploadCount,
			_nextChunkIndex);
	if (_nextFilterIndex == _filterUploadCount) {
		// We're done
		commit();
		return;
	}

	std::optional<uint8_t> index = _store->findFilterIndex(_filterIdsToUpload[_nextFilterIndex]);
	if (!index.has_value()) {
		reset();
		return;
	}
	AssetFilter filter = _store->getFilter(index.value());

	event_t eventGetWriteBuf(CS_TYPE::CMD_CS_CENTRAL_GET_WRITE_BUF);
	eventGetWriteBuf.dispatch();
	cs_data_t writeBuf = eventGetWriteBuf.result.buf;
	LOGAssetFilterSyncerVerbose("writeBuf size=%u", writeBuf.len);
	if (writeBuf.data == nullptr) {
		reset();
		return;
	}

	if (writeBuf.len < sizeof(asset_filter_cmd_upload_filter_t)) {
		reset();
		return;
	}

	uint16_t maxChunkSize = writeBuf.len - sizeof(asset_filter_cmd_upload_filter_t);
	if (maxChunkSize == 0) {
		reset();
		return;
	}

	uint16_t filterDataLength = filter.filterdata().length();
	uint16_t filterDataRemaining = filterDataLength - _nextChunkIndex;
	uint16_t chunkSize = std::min(maxChunkSize, filterDataRemaining);
	LOGAssetFilterSyncerVerbose("maxChunkSize=%u filterDataLength=%u filterDataRemaining=%u chunkSize=%u",
			maxChunkSize,
			filterDataLength,
			filterDataRemaining,
			chunkSize);

	auto uploadCmd = reinterpret_cast<asset_filter_cmd_upload_filter_t*>(writeBuf.data);
	uploadCmd->protocolVersion = ASSET_FILTER_CMD_PROTOCOL_VERSION;
	uploadCmd->filterId = filter.runtimedata()->filterId;
	uploadCmd->chunkStartIndex = _nextChunkIndex;
	uploadCmd->totalSize = filterDataLength;
	uploadCmd->chunkSize = chunkSize;
	memcpy(uploadCmd->chunk, filter.filterdata()._data + _nextChunkIndex, chunkSize);

	TYPIFY(CMD_CS_CENTRAL_WRITE) packet;
	packet.commandType = CTRL_CMD_FILTER_UPLOAD;
	packet.data = cs_data_t(reinterpret_cast<uint8_t*>(uploadCmd), sizeof(asset_filter_cmd_upload_filter_t) + uploadCmd->chunkSize);

	event_t event(CS_TYPE::CMD_CS_CENTRAL_WRITE, &packet, sizeof(packet));
	event.dispatch();
	if (event.result.returnCode != ERR_WAIT_FOR_SUCCESS) {
		reset();
		return;
	}
	setStep(SyncStep::UPLOAD_FILTERS);

	_nextChunkIndex += chunkSize;
	if (_nextChunkIndex == filterDataLength) {
		_nextChunkIndex = 0;
		_nextFilterIndex++;
	}
}

void AssetFilterSyncer::commit() {
	LOGAssetFilterSyncerDebug("commit");
	asset_filter_cmd_commit_filter_changes_t commitCmd = {
			.protocolVersion = ASSET_FILTER_CMD_PROTOCOL_VERSION,
			.masterVersion = _store->getMasterVersion(),
			.masterCrc = _store->getMasterCrc()
	};

	TYPIFY(CMD_CS_CENTRAL_WRITE) packet;
	packet.commandType = CTRL_CMD_FILTER_COMMIT;
	packet.data = cs_data_t(reinterpret_cast<uint8_t*>(&commitCmd), sizeof(commitCmd));

	event_t event(CS_TYPE::CMD_CS_CENTRAL_WRITE, &packet, sizeof(packet));
	event.dispatch();
	if (event.result.returnCode != ERR_WAIT_FOR_SUCCESS) {
		reset();
		return;
	}

	setStep(SyncStep::COMMIT);
}

void AssetFilterSyncer::disconnect() {
	LOGAssetFilterSyncerDebug("disconnect");
	event_t event(CS_TYPE::CMD_CS_CENTRAL_DISCONNECT);
	event.dispatch();
	switch (event.result.returnCode) {
		case ERR_SUCCESS: {
			// Already disconnected.
			setStep(SyncStep::NONE);
			done();
			return;
		}
		case ERR_WAIT_FOR_SUCCESS: {
			setStep(SyncStep::DISCONNECT);
			return;
		}
		default: {
			LOGw("Disconnect failed");
			setStep(SyncStep::NONE);
			return;
		}
	}
}

void AssetFilterSyncer::done() {
	LOGAssetFilterSyncerInfo("Done uploading master version %u", _store->getMasterVersion());
	// Send out version again, so the next crownstone with an old version can send their version, which makes us connect to that crownstone.
	sendVersionAtLowInterval();
}



void AssetFilterSyncer::onConnectResult(cs_ret_code_t retCode) {
	if (_step != SyncStep::CONNECT) {
		return;
	}
	if (retCode != ERR_SUCCESS) {
		reset();
		return;
	}

	// Get filter summaries.
	TYPIFY(CMD_CS_CENTRAL_WRITE) packet;
	packet.commandType = CTRL_CMD_FILTER_GET_SUMMARIES;
	event_t event(CS_TYPE::CMD_CS_CENTRAL_WRITE, &packet, sizeof(packet));
	event.dispatch();
	if (event.result.returnCode != ERR_WAIT_FOR_SUCCESS) {
		reset();
		return;
	}
	setStep(SyncStep::GET_FILTER_SUMMARIES);
}

void AssetFilterSyncer::onDisconnect() {
	if (_step == SyncStep::DISCONNECT) {
		done();
	}
	if (_step != SyncStep::NONE) {
		reset();
	}
}

void AssetFilterSyncer::onWriteResult(cs_central_write_result_t& result) {
	if (_step == SyncStep::NONE) {
		return;
	}

	// Initial checks, the order matters.
	LOGAssetFilterSyncerDebug("onWriteResult writeRetCode=%u step=%u", _step);
	if (result.writeRetCode != ERR_SUCCESS || !result.result.isInitialized()) {
		reset();
		return;
	}

	LOGAssetFilterSyncerDebug("  protocol=%u resultCode=%u type=%u",
			result.result.getProtocolVersion(),
			result.result.getResult(),
			result.result.getType());
	if (result.result.getProtocolVersion() != CS_CONNECTION_PROTOCOL_VERSION || result.result.getResult() != ERR_SUCCESS) {
		reset();
		return;
	}

	auto payload = result.result.getPayload();
	switch (_step) {
		case SyncStep::GET_FILTER_SUMMARIES: {
			if (result.result.getType() != CTRL_CMD_FILTER_GET_SUMMARIES) {
				reset();
				return;
			}
			onFilterSummaries(payload);
			break;
		}
		case SyncStep::REMOVE_FILTERS: {
			if (result.result.getType() != CTRL_CMD_FILTER_REMOVE) {
				reset();
				return;
			}
			removeNextFilter();
			break;
		}
		case SyncStep::UPLOAD_FILTERS: {
			if (result.result.getType() != CTRL_CMD_FILTER_UPLOAD) {
				reset();
				return;
			}
			uploadNextFilter();
			break;
		}
		case SyncStep::COMMIT: {
			if (result.result.getType() != CTRL_CMD_FILTER_COMMIT) {
				reset();
				return;
			}
			disconnect();
			break;
		}
		default: {
			reset();
			return;
		}
	}
}

void AssetFilterSyncer::onFilterSummaries(cs_data_t& payload) {
	LOGAssetFilterSyncerDebug("onFilterSummaries");
	if (payload.len < sizeof(asset_filter_cmd_get_filter_summaries_ret_t)) {
		reset();
		return;
	}
	auto header = reinterpret_cast<asset_filter_cmd_get_filter_summaries_ret_t*>(payload.data);

	// Double check master version etc.
	if (compareToMyVersion(header->protocolVersion, header->masterVersion, header->masterCrc) != VersionCompare::OLDER) {
		reset();
		return;
	}

	// Figure out which filter IDs to upload, and which to remove.
	_filterUploadCount = 0;
	_filterRemoveCount = 0;

	// Loop over their filters, to see if there are abundant IDs, or filter CRC mismatches.
	uint8_t filterCount = (payload.len - sizeof(*header)) / sizeof(asset_filter_summary_t);
	for (uint8_t i = 0; i < filterCount; ++i) {
		uint8_t filterId = header->summaries[i].id;
		std::optional<uint8_t> index = _store->findFilterIndex(filterId);
		if (index.has_value()) {
			AssetFilter myFilter = _store->getFilter(index.value());
			if (myFilter.runtimedata()->crc != header->summaries[i].crc) {
				LOGAssetFilterSyncerVerbose("CRC mismatch, upload filterId=%u", filterId);
				_filterIdsToUpload[_filterUploadCount++] = filterId;
			}
			else {
				LOGAssetFilterSyncerVerbose("CRC match, skip filterId=%u", filterId);
			}
		}
		else {
			_filterIdsToRemove[_filterRemoveCount++] = filterId;
			LOGAssetFilterSyncerVerbose("Not found, remove filterId=%u", filterId);
		}
	}

	// Loop over our filters, to see if there are IDs that we have, but the other does not.
	for (uint8_t i = 0; i < _store->getFilterCount(); ++i) {
		AssetFilter myFilter = _store->getFilter(i);
		uint8_t filterId = myFilter.runtimedata()->filterId;
		bool found = false;
		for (uint8_t j = 0; j < filterCount; ++j) {
			if (header->summaries[j].id == filterId) {
				// CRC has been checked in previous loop.
				found = true;
				break;
			}
		}
		if (!found) {
			LOGAssetFilterSyncerVerbose("Missing, upload filterId=%u", filterId);
			_filterIdsToUpload[_filterUploadCount++] = filterId;
		}
	}

	_nextFilterIndex = 0;
	_nextChunkIndex = 0;
	setStep(SyncStep::REMOVE_FILTERS);
	removeNextFilter();
}

void AssetFilterSyncer::onModificationInProgress(bool inProgress) {
	LOGAssetFilterSyncerDebug("onModificationInProgress %u", inProgress);
	if (inProgress && _step != SyncStep::NONE) {
		// Abort the current upload.
		reset();
	}
}

void AssetFilterSyncer::sendVersionAtLowInterval() {
	_sendVersionAtLowIntervalCountdown = VERSION_BROADCAST_INTERVAL_RESET_SECONDS * 1000 / TICK_INTERVAL_MS;

	// Clamp the countdown: leave current countdown as it is, but it should be at most the low interval.
	_sendVersionCountdown = CsMath::min(
			_sendVersionCountdown,
			VERSION_BROADCAST_LOW_INTERVAL_SECONDS * 1000 / TICK_INTERVAL_MS);
	LOGAssetFilterSyncerDebug("sendVersionAtLowInterval sendVersionCountdown=%u", _sendVersionCountdown);
}

void AssetFilterSyncer::onTick(uint32_t tickCount) {
	// Decrement timers.
	if (_sendVersionAtLowIntervalCountdown != 0 ) {
		_sendVersionAtLowIntervalCountdown--;
	}
	if (_sendVersionCountdown != 0) {
		_sendVersionCountdown--;
	}

	if (_sendVersionCountdown == 0) {
		// Send our version.
		bool isInLowIntervalMode = _sendVersionAtLowIntervalCountdown == 0;

		_sendVersionCountdown = isInLowIntervalMode
				? VERSION_BROADCAST_NORMAL_INTERVAL_SECONDS * 1000 / TICK_INTERVAL_MS
				: VERSION_BROADCAST_LOW_INTERVAL_SECONDS * 1000 / TICK_INTERVAL_MS;
		sendVersion(false);
	}
}

void AssetFilterSyncer::handleEvent(event_t& event) {
	switch (event.type) {
		case CS_TYPE::EVT_RECV_MESH_MSG: {
			auto meshMsg = CS_TYPE_CAST(EVT_RECV_MESH_MSG, event.data);
			if (meshMsg->type == CS_MESH_MODEL_TYPE_ASSET_FILTER_VERSION && meshMsg->hops == 0) {
				// Only handle 0 hop messages, as in order to sync, we need to connect.
				auto packet = meshMsg->getPacket<CS_MESH_MODEL_TYPE_ASSET_FILTER_VERSION>();
				event.result.returnCode = onVersion(meshMsg->srcAddress, packet);
			}
			break;
		}
		case CS_TYPE::EVT_FILTERS_UPDATED: {
			sendVersionAtLowInterval();
			break;
		}
		case CS_TYPE::EVT_FILTER_MODIFICATION: {
			auto packet = CS_TYPE_CAST(EVT_FILTER_MODIFICATION, event.data);
			onModificationInProgress(*packet);
			break;
		}
		case CS_TYPE::EVT_CS_CENTRAL_CONNECT_RESULT: {
			auto result = CS_TYPE_CAST(EVT_CS_CENTRAL_CONNECT_RESULT, event.data);
			onConnectResult(*result);
			break;
		}
		case CS_TYPE::EVT_CS_CENTRAL_WRITE_RESULT: {
			auto result = CS_TYPE_CAST(EVT_CS_CENTRAL_WRITE_RESULT, event.data);
			onWriteResult(*result);
			break;
		}
		case CS_TYPE::EVT_BLE_CENTRAL_DISCONNECTED: {
			onDisconnect();
			break;
		}
		case CS_TYPE::EVT_TICK: {
			auto tickCount = CS_TYPE_CAST(EVT_TICK, event.data);
			onTick(*tickCount);
			break;
		}
		default:
			break;
	}
}
