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
#include <localisation/cs_TrackableId.h>
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

#define LOGNearestCrownstoneTrackerVerbose LOGvv
#define LOGNearestCrownstoneTrackerDebug LOGvv
#define LOGNearestCrownstoneTrackerInfo LOGvv

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

	if (meshMsgEvent->type == CS_MESH_MODEL_TYPE_REPORT_ASSET_ID) {
		LOGNearestCrownstoneTrackerVerbose("NearestCrownstone received REPORT_ASSET_ID");

		onReceiveAssetReport(
				meshMsgEvent->getPacket<CS_MESH_MODEL_TYPE_REPORT_ASSET_ID>(),
				meshMsgEvent->srcAddress);

		evt.result = ERR_SUCCESS;
	}
}

uint16_t NearestCrownstoneTracker::handleAcceptedAsset(const scanned_device_t& asset, const short_asset_id_t& id) {
	report_asset_id_t rep = {};
	rep.id = id;
	rep.compressedRssi = rssi_and_channel_t(asset.rssi,asset.channel);
	onReceiveAssetAdvertisement(rep);
	return MIN_THROTTLED_ADVERTISEMENT_PERIOD_MS;
}


void NearestCrownstoneTracker::onReceiveAssetAdvertisement(report_asset_id_t& incomingReport) {
	LOGNearestCrownstoneTrackerVerbose("onReceiveAssetAdvertisement myId(%u), report(%d dB ch.%u)",
			_myStoneId, getRssi(incomingReport.compressedRssi), getChannel(incomingReport.compressedRssi));

	auto recordPtr = getRecordFiltered(incomingReport.id);
	if (recordPtr == nullptr) {
		// might just have been an old record. simply return.
		return;
	}
	auto& record = *recordPtr;

	if (record.nearestStoneId == 0 || record.nearestStoneId == _myStoneId) {
		if (record.nearestStoneId == 0) {
			LOGNearestCrownstoneTrackerDebug("First time this asset was seen, consider us nearest.");
			onWinnerChanged(true);
		}
		else {
			LOGNearestCrownstoneTrackerVerbose("We already believed we were nearest, so it's time to send an update towards the mesh");
		}
		saveWinningReport(record, incomingReport.compressedRssi, _myStoneId);
		broadcastReport(incomingReport);
		sendUartUpdate(record);
	}
	else {
		LOGNearestCrownstoneTrackerVerbose("We didn't win before");
		if (incomingReport.compressedRssi.isCloserThan(record.nearestRssi))  {
			// we win because the incoming report is a first hand observation.
			LOGNearestCrownstoneTrackerVerbose("but now we do, so have to send an update towards the mesh");
			saveWinningReport(record, incomingReport.compressedRssi, _myStoneId);
			broadcastReport(incomingReport);
			sendUartUpdate(record);
			onWinnerChanged(true);
		}
		else {
			LOGNearestCrownstoneTrackerVerbose("We still don't, so we're done.");
		}
	}
}

void NearestCrownstoneTracker::onReceiveAssetReport(report_asset_id_t& incomingReport, stone_id_t reporter) {
	LOGNearestCrownstoneTrackerVerbose("onReceive witness report, myId(%u), reporter(%u), rssi(%i #%u)",
			_myStoneId, reporter, getRssi(incomingReport.compressedRssi), getChannel(incomingReport.compressedRssi));

	auto recordPtr = getRecordFiltered(incomingReport.id);
	if (recordPtr == nullptr) {
		// if we don't have a record in the assetStore it means we're not close
		// and can simply ignore the message.
		return;
	}
	auto& record = *recordPtr;

	// REVIEW: doesn't use the RSSI with fall off.
	if (reporter == record.nearestStoneId) {
		LOGNearestCrownstoneTrackerVerbose("Received an update from the winner.");

		if (record.myRssi.isCloserThan(incomingReport.compressedRssi) ) {
			LOGNearestCrownstoneTrackerVerbose("It dropped below my own value, so I win now.");
			saveWinningReport(record, record.myRssi, _myStoneId);

			// REVIEW: this report is outdated. Also, it may lead to many messages being sent.
			// ANSWER(Arend): this is the first moment we can react to a change in nearest-status. We could:
			//   - wait for another advertisement
			//   - take fall-off into account.
			broadcastPersonalReport(record);
			sendUartUpdate(record);		// TODO: move into AssetFiltering to prevent duplicate messages on UART
			onWinnerChanged(true);
		}
		else {
			LOGNearestCrownstoneTrackerVerbose("It still wins, so I'll just update the value of my winning report.");
			saveWinningReport(record, incomingReport.compressedRssi, reporter);
			sendUartUpdate(record);
		}
	}
	else {
		if (record.nearestStoneId == 0 || incomingReport.compressedRssi.isCloserThan(record.nearestRssi)) {
			LOGNearestCrownstoneTrackerVerbose("Received a witnessreport from another crownstone that is better than my winner.");
			saveWinningReport(record, incomingReport.compressedRssi, reporter);
			sendUartUpdate(record);
			onWinnerChanged(false);
		}
	}
}

// -------------------------------------------
// ------------- Outgoing events -------------
// -------------------------------------------


void NearestCrownstoneTracker::broadcastReport(report_asset_id_t& report) {

	cs_mesh_msg_t reportMsgWrapper;
	reportMsgWrapper.type =  CS_MESH_MODEL_TYPE_REPORT_ASSET_ID;
	reportMsgWrapper.payload = reinterpret_cast<uint8_t*>(&report);
	reportMsgWrapper.size = sizeof(report);
	reportMsgWrapper.reliability = CS_MESH_RELIABILITY_LOW;
	reportMsgWrapper.urgency = CS_MESH_URGENCY_LOW;

	event_t reportMsgEvt(CS_TYPE::CMD_SEND_MESH_MSG, &reportMsgWrapper, sizeof(reportMsgWrapper));

	reportMsgEvt.dispatch();
}

void NearestCrownstoneTracker::broadcastPersonalReport(asset_record_t& record) {
	report_asset_id_t report = {};
	report.id = record.assetId;
	report.compressedRssi = record.myRssi;

	broadcastReport(report);
}

void NearestCrownstoneTracker::sendUartUpdate(asset_record_t& record) {
	auto uartMsg = cs_asset_rssi_data_sid_t{
			.assetId = record.assetId,
			.stoneId = record.nearestStoneId,
			.rssi = record.nearestRssi.getRssi(),
			.channel = record.nearestRssi.getChannel()
	};

	// REVIEW: doesn't have to be a different message than asset id report.
	// ANSWER(Arend): adjusted, but I'm suggesting further refactor
	UartHandler::getInstance().writeMsg(
			UartOpcodeTx::UART_OPCODE_TX_ASSET_RSSI_SID_DATA,
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
	rec.nearestRssi = winningRssi;
}

asset_record_t* NearestCrownstoneTracker::getRecordFiltered(const short_asset_id_t& assetId) {
	asset_record_t* record = _assetStore->getRecord(assetId);

	if (record == nullptr) {
		return nullptr;
	}

	// REVIEW: why is there "constexpr" here? Seems like something the compiler easily can find out by itsself.
	// ANSWER(Arend): constexpr guarantees it. standard if statement isn't required to discard unused branch
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


