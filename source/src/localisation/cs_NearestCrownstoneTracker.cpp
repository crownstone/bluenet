/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 17, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <common/cs_Types.h>
#include <events/cs_Event.h>
#include <localisation/cs_NearestCrownstoneTracker.h>
#include <localisation/cs_TrackableEvent.h>

#include <logging/cs_Logger.h>
#include <protocol/mesh/cs_MeshModelPackets.h>
#include <protocol/cs_UartOpcodes.h>
#include <protocol/cs_Packets.h>
#include <storage/cs_State.h>
#include <util/cs_Coroutine.h>
#include <uart/cs_UartHandler.h>
#include <protocol/cs_RssiAndChannel.h>

#include <localisation/cs_AssetFilterStore.h>
#include <common/cs_Component.h>

#define LOGNearestCrownstoneTrackerVerbose LOGd
#define LOGNearestCrownstoneTrackerDebug LOGd
#define LOGNearestCrownstoneTrackerInfo LOGd

cs_ret_code_t NearestCrownstoneTracker::init() {
	State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &_myStoneId, sizeof(_myStoneId));

	_assetStore = getComponent<AssetStore>();
	if (_assetStore == nullptr) {
		LOGd("no asset store found, Nearest crownstone refuses init.");
		return ERR_NOT_FOUND;
	}

	listen();

	return ERR_SUCCESS;
}

// -------------------------------------------
// ------------- Incoming events -------------
// -------------------------------------------

void NearestCrownstoneTracker::handleEvent(event_t &evt) {
	switch(evt.type) {
		case CS_TYPE::EVT_RECV_MESH_MSG: {
			handleMeshMsgEvent(evt);
			break;
		}
		default: {
			break;
		}
	}
}

void NearestCrownstoneTracker::handleMeshMsgEvent(event_t& evt) {
	// transform to representation used in NearestCrownstoneTracker.
	MeshMsgEvent* meshMsgEvent = CS_TYPE_CAST(EVT_RECV_MESH_MSG, evt.data);

	if (meshMsgEvent->type == CS_MESH_MODEL_TYPE_ASSET_INFO_ID) {
		LOGNearestCrownstoneTrackerVerbose("NearestCrownstone received REPORT_ASSET_ID");

		onReceiveAssetReport(
				meshMsgEvent->getPacket<CS_MESH_MODEL_TYPE_ASSET_INFO_ID>(),
				meshMsgEvent->srcAddress);

		evt.result = ERR_SUCCESS;
	}
}

uint16_t NearestCrownstoneTracker::handleAcceptedAsset(const scanned_device_t& asset, const asset_id_t& id, uint8_t filterBitmask) {
	LOGNearestCrownstoneTrackerVerbose("handleAcceptedAsset");
	auto assetMsg = cs_mesh_model_msg_asset_report_id_t{
		.id            = id,
		.filterBitmask = filterBitmask,
		.rssi      = asset.rssi,
		.channel = 0
	};

	assetMsg.channel = compressChannel(asset.channel);

	onReceiveAssetAdvertisement(assetMsg);
	return MIN_THROTTLED_ADVERTISEMENT_PERIOD_MS;
}


bool NearestCrownstoneTracker::onReceiveAssetAdvertisement(cs_mesh_model_msg_asset_report_id_t& incomingReport) {
	LOGNearestCrownstoneTrackerVerbose("onReceiveAssetAdvertisement myId(%u), report(%d dB ch.%u)",
			_myStoneId, incomingReport.rssi, decompressChannel(incomingReport.channel));

	auto recordPtr = getRecordFiltered(incomingReport.id);
	if (recordPtr == nullptr) {
		// might just have been an old record. no further action required.
		return false;
	}
	auto& record = *recordPtr;

	auto incomingRssiAndChannelCompressed = rssi_and_channel_t(incomingReport.rssi, decompressChannel(incomingReport.channel));
	auto incomingRssiAndChannel = incomingRssiAndChannelCompressed.toFloat();

	auto recordedNearestRssiWithFallOff = rssi_and_channel_float_t(record.nearestRssi).fallOff(
			RSSI_FALL_OFF_RATE_DB_PER_S, record.lastReceivedCounter *
			1e-3f * AssetStore::LAST_RECEIVED_COUNTER_PERIOD_MS);

	if (record.nearestStoneId == 0) {
		LOGNearestCrownstoneTrackerDebug("First time this asset was seen, consider us nearest.");
		onWinnerChanged(true);
		return true;
	}

	if (record.nearestStoneId == _myStoneId) {
		LOGNearestCrownstoneTrackerVerbose("We already believed we were nearest");
		saveWinningReport(record, incomingRssiAndChannelCompressed, _myStoneId);
		return true;
	}

	LOGNearestCrownstoneTrackerVerbose("We didn't win before");
	if (incomingRssiAndChannel.isCloserThan(recordedNearestRssiWithFallOff))  {
		LOGNearestCrownstoneTrackerVerbose("but now we do");
		saveWinningReport(record, incomingRssiAndChannelCompressed, _myStoneId);
		onWinnerChanged(true);
		return true;
	}

	LOGNearestCrownstoneTrackerVerbose("We still don't");
	return false;
}

void NearestCrownstoneTracker::onReceiveAssetReport(cs_mesh_model_msg_asset_report_id_t& incomingReport, stone_id_t reporter) {
	LOGNearestCrownstoneTrackerVerbose("onReceive witness report, myId(%u), reporter(%u), rssi(%i #%u)",
			_myStoneId, reporter, incomingReport.rssi, decompressChannel(incomingReport.channel));

	auto recordPtr = getRecordFiltered(incomingReport.id);
	if (recordPtr == nullptr) {
		// if we don't have a record in the assetStore it means we're not close
		// and can simply ignore the message.
		return;
	}
	auto& record = *recordPtr;

	auto incomingRssiAndChannelCompressed = rssi_and_channel_t(incomingReport.rssi, decompressChannel(incomingReport.channel));
	auto incomingRssiAndChannel = incomingRssiAndChannelCompressed.toFloat();

	auto recordedNearestRssiWithFallOff = rssi_and_channel_float_t(record.nearestRssi).fallOff(
			RSSI_FALL_OFF_RATE_DB_PER_S, record.lastReceivedCounter *
			1e-3f * AssetStore::LAST_RECEIVED_COUNTER_PERIOD_MS);
	auto recordedPersonalRssiWithFallOff = record.myRssi.fallOff(
				RSSI_FALL_OFF_RATE_DB_PER_S, record.lastReceivedCounter *
				1e-3f * AssetStore::LAST_RECEIVED_COUNTER_PERIOD_MS);

	if (reporter == record.nearestStoneId) {
		LOGNearestCrownstoneTrackerVerbose("Received an update from the winner.");

		if (recordedPersonalRssiWithFallOff.isCloserThan(incomingRssiAndChannel) ) {
			LOGNearestCrownstoneTrackerVerbose("It dropped below my own value, so I win now.");
			saveWinningReport(record, record.myRssi, _myStoneId);

			sendUartUpdate(record);
			onWinnerChanged(true);
		}
		else {
			LOGNearestCrownstoneTrackerVerbose("It still wins, so I'll just update the value of my winning report.");
			saveWinningReport(record, incomingRssiAndChannelCompressed, reporter);
			sendUartUpdate(record);
		}
	}
	else {
		if (record.nearestStoneId == 0 || incomingRssiAndChannel.isCloserThan(recordedNearestRssiWithFallOff)) {
			LOGNearestCrownstoneTrackerVerbose("Received a witnessreport from another crownstone that is better than my winner.");
			saveWinningReport(record, incomingRssiAndChannelCompressed, reporter);
			sendUartUpdate(record);
			onWinnerChanged(false);
		}
	}
}

// -------------------------------------------
// ------------- Outgoing events -------------
// -------------------------------------------


void NearestCrownstoneTracker::broadcastReport(cs_mesh_model_msg_asset_report_id_t& report) {

	cs_mesh_msg_t reportMsgWrapper;
	reportMsgWrapper.type =  CS_MESH_MODEL_TYPE_ASSET_INFO_ID;
	reportMsgWrapper.payload = reinterpret_cast<uint8_t*>(&report);
	reportMsgWrapper.size = sizeof(report);
	reportMsgWrapper.reliability = CS_MESH_RELIABILITY_LOW;
	reportMsgWrapper.urgency = CS_MESH_URGENCY_LOW;

	event_t reportMsgEvt(CS_TYPE::CMD_SEND_MESH_MSG, &reportMsgWrapper, sizeof(reportMsgWrapper));

	reportMsgEvt.dispatch();
}

void NearestCrownstoneTracker::broadcastPersonalReport(asset_record_t& record) {
	cs_mesh_model_msg_asset_report_id_t report = {};
	report.id = record.assetId;
	report.rssi = record.myRssi.getRssi();

	broadcastReport(report);
}

void NearestCrownstoneTracker::sendUartUpdate(asset_record_t& record) {
	auto uartMsg = asset_report_uart_id_t{
			.assetId = record.assetId,
			.stoneId = record.nearestStoneId,
			.rssi    = record.nearestRssi,
			.channel = 0
	};

	UartHandler::getInstance().writeMsg(
			UartOpcodeTx::UART_OPCODE_TX_ASSET_INFO_SID,
			reinterpret_cast<uint8_t*>(&uartMsg),
			sizeof(uartMsg));
}

void NearestCrownstoneTracker::onWinnerChanged(bool winnerIsThisCrownstone) {
	LOGd("Nearest changed. Winner is this crownstone: %u",	winnerIsThisCrownstone);
}


// ---------------------------------
// ------------- Utils -------------
// ---------------------------------


void NearestCrownstoneTracker::saveWinningReport(asset_record_t& rec, rssi_and_channel_t winningRssi, stone_id_t winningId) {
	rec.nearestStoneId = winningId;
	rec.nearestRssi = winningRssi.getRssi();
}

asset_record_t* NearestCrownstoneTracker::getRecordFiltered(const asset_id_t& assetId) {
	asset_record_t* record = _assetStore->getRecord(assetId);

	if (record == nullptr) {
		return nullptr;
	}

	if constexpr (FILTER_STRATEGY == FilterStrategy::TIME_OUT) {
		if (record->lastReceivedCounter >= LAST_RECEIVED_TIMEOUT_THRESHOLD) {
			LOGd("ignored old record for nearest crownstone algorithm.");
			return nullptr;
		}
	}
	else if constexpr (FILTER_STRATEGY == FilterStrategy::RSSI_FALL_OFF) {
		auto correctedRssi = record->myRssi.getRssi()
							 - (record->lastReceivedCounter * RSSI_FALL_OFF_RATE_DB_PER_S * 1000)
									   / AssetStore::LAST_RECEIVED_COUNTER_PERIOD_MS;

		if (correctedRssi < RSSI_CUT_OFF_THRESHOLD) {
			return nullptr;
		}
	}

	return record;
}


