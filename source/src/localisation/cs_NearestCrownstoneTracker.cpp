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
#include <util/cs_Rssi.h>

#include <localisation/cs_AssetFilterStore.h>
#include <common/cs_Component.h>

#define LOGNearestCrownstoneTrackerVerbose LOGvv
#define LOGNearestCrownstoneTrackerDebug LOGvv
#define LOGNearestCrownstoneTrackerInfo LOGvv

cs_ret_code_t NearestCrownstoneTracker::init() {
	State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &_myId, sizeof(_myId));

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
		case CS_TYPE::EVT_ASSET_ACCEPTED_FOR_NEAREST_ALGORITHM: {
			handleAssetAcceptedEvent(evt);
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

void NearestCrownstoneTracker::handleAssetAcceptedEvent(event_t& evt){
	// an asset advertisement passed this crownstones filters.
	AssetWithSidAcceptedEvent* assetAcceptedEvent = CS_TYPE_CAST(EVT_ASSET_ACCEPTED_FOR_NEAREST_ALGORITHM, evt.data);

	report_asset_id_t rep = {};
	rep.id = assetAcceptedEvent->_id;
	rep.compressedRssi = compressRssi(
					assetAcceptedEvent->_asset.rssi,
					assetAcceptedEvent->_asset.channel);

	onReceiveAssetAdvertisement(rep);

	evt.result = ERR_SUCCESS;
}


void NearestCrownstoneTracker::onReceiveAssetAdvertisement(report_asset_id_t& incomingReport) {
	LOGNearestCrownstoneTrackerVerbose("onReceive trackable, myId(%u), report(%d dB ch.%u)",
			_myId, getRssi(incomingReport.compressedRssi), getChannel(incomingReport.compressedRssi));

	auto recordPtr = _assetStore->getOrCreateRecord(incomingReport.id);
	if (recordPtr == nullptr) {
		// (warning is already logged in getOrCreateRecord)
		return;
	}
	auto& record = *recordPtr;

	savePersonalReport(record, incomingReport.compressedRssi);

	// REVIEW: Does it matter who reported it?
	// @Bart: Yes. The crownstone always saves a 'personal report', but the rssi value between
	// this crownstone and the trackable only becomes relevant to other crownstones when it is (or becomes)
	// the new nearest. Those are the cases you see:
	// - we are already the nearest. Then all updates are relevant.
	// - we are becoming the nearest. Then 'winner' changes, and we start updating towards the mesh.
	// - we are not the nearest. Then we don't need to do anything beyond keeping track of our own distance to the trackable.
	//   (Which was already done before the ifstatement.)
	if (record.nearestStoneId == _myId) {
		LOGNearestCrownstoneTrackerVerbose("we already believed we were nearest, so it's time to send an update towards the mesh");

		saveWinningReport(record, incomingReport.compressedRssi, _myId);

		broadcastReport(incomingReport);

		sendUartUpdate(record);
	}
	else {
		LOGNearestCrownstoneTrackerVerbose("we didn't win before");
		if (rssiIsCloser(incomingReport.compressedRssi, record.nearestRssi))  {
			// we win because the incoming report is a first hand observation.
			LOGNearestCrownstoneTrackerVerbose("but now we do, so have to send an update towards the mesh");
			saveWinningReport(record, incomingReport.compressedRssi, _myId);

			broadcastReport(incomingReport);

			sendUartUpdate(record);

			onWinnerChanged(true);
		}
		else {
			LOGNearestCrownstoneTrackerVerbose("we still don't, so we're done.");
		}
	}
}

void NearestCrownstoneTracker::onReceiveAssetReport(report_asset_id_t& incomingReport, stone_id_t reporter) {
	LOGNearestCrownstoneTrackerVerbose("onReceive witness report, myId(%u), reporter(%u), rssi(%i #%u)",
			_myId, reporter, getRssi(incomingReport.compressedRssi), getChannel(incomingReport.compressedRssi));

	if (reporter == _myId) {
		LOGNearestCrownstoneTrackerVerbose("Received an old report from myself. Dropped: not relevant.");
		return;
	}

	auto recordPtr = _assetStore->getOrCreateRecord(incomingReport.id);
	if (recordPtr == nullptr) {
		// (warning is already logged in getOrCreateRecord)
		return;
	}
	auto& record = *recordPtr;

	if (reporter == record.nearestStoneId) {
		LOGNearestCrownstoneTrackerVerbose("Received an update from the winner.");

		if (rssiIsCloser(record.myRssi, incomingReport.compressedRssi) ) {
			LOGNearestCrownstoneTrackerVerbose("It dropped below my own value, so I win now. ");
			saveWinningReport(record, record.myRssi, _myId);

			LOGNearestCrownstoneTrackerVerbose("Broadcast my personal report to update the mesh.");
			broadcastPersonalReport(record);

			sendUartUpdate(record);

			onWinnerChanged(true);
		} else {
			LOGNearestCrownstoneTrackerVerbose("It still wins, so I'll just update the value of my winning report.");
			saveWinningReport(record, incomingReport.compressedRssi, reporter);

			sendUartUpdate(record);
		}
	}
	else {
		if (rssiIsCloser(incomingReport.compressedRssi, record.nearestRssi)) {
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
	auto uartMsg = cs_nearest_stone_update_t{
			.assetId = record.assetId,
			.stoneId = record.nearestStoneId,
			.rssi = getRssi(record.nearestRssi),
			.channel = getChannel(record.nearestRssi)
	};

	UartHandler::getInstance().writeMsg(
			UartOpcodeTx::UART_OPCODE_TX_NEAREST_CROWNSTONE_UPDATE,
			reinterpret_cast<uint8_t*>(&uartMsg),
			sizeof(uartMsg));
}


void NearestCrownstoneTracker::sendUartTimeout(const short_asset_id_t& assetId) {
	auto uartMsg = cs_nearest_stone_timeout_t{
			.assetId = assetId
	};

	UartHandler::getInstance().writeMsg(
			UartOpcodeTx::UART_OPCODE_TX_NEAREST_CROWNSTONE_TIMEOUT,
			reinterpret_cast<uint8_t*>(&uartMsg),
			sizeof(uartMsg));
}

void NearestCrownstoneTracker::onWinnerChanged(bool winnerIsThisCrownstone) {
	LOGd("Nearest changed. I'm turning %s",
			winnerIsThisCrownstone ? "on" : "off");

	CS_TYPE onOff = winnerIsThisCrownstone
			? CS_TYPE::CMD_SWITCH_ON
			: CS_TYPE::CMD_SWITCH_OFF;

	event_t event(onOff, nullptr, 0, cmd_source_t(CS_CMD_SOURCE_SWITCHCRAFT));
	event.dispatch();
}


// ---------------------------------
// ------------- Utils -------------
// ---------------------------------


void NearestCrownstoneTracker::savePersonalReport(asset_record_t& rec, compressed_rssi_data_t personalRssi) {
	rec.myRssi = personalRssi;
}

void NearestCrownstoneTracker::saveWinningReport(asset_record_t& rec, compressed_rssi_data_t winningRssi, stone_id_t winningId) {
	rec.nearestStoneId = winningId;
	rec.nearestRssi = winningRssi;
}


