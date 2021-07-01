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
#include <storage/cs_State.h>
#include <util/cs_Coroutine.h>

#define LOGNearestCrownstoneTrackerVerbose LOGnone
#define LOGNearestCrownstoneTrackerDebug LOGnone
#define LOGNearestCrownstoneTrackerInfo LOGnone

void NearestCrownstoneTracker::init() {
	State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &_myId, sizeof(_myId));

	resetReports();
}

void NearestCrownstoneTracker::handleEvent(event_t &evt) {
	switch(evt.type) {
		case CS_TYPE::EVT_RECV_MESH_MSG: {
			handleMeshMsgEvent(evt);
			break;
		}
		case CS_TYPE::EVT_ASSET_ACCEPTED: {
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
	AssetAcceptedEvent* assetAcceptedEvent = CS_TYPE_CAST(EVT_ASSET_ACCEPTED, evt.data);

	report_asset_id_t rep = {};
	rep.id = assetAcceptedEvent->_id;
	rep.rssi = assetAcceptedEvent->_asset.rssi;

	onReceiveAssetAdvertisement(rep);
}


void NearestCrownstoneTracker::onReceiveAssetAdvertisement(report_asset_id_t& incomingReport) {
	LOGNearestCrownstoneTrackerVerbose("onReceive trackable, myId(%u)", _myId);

	savePersonalReport(incomingReport);

	auto& _winningReport = getWinningReport(incomingReport.id);

	// REVIEW: Does it matter who reported it?
	// @Bart: Yes. The crownstone always saves a 'personal report', but the rssi value between
	// this crownstone and the trackable only becomes relevant to other crownstones when it is (or becomes)
	// the new closest. Those are the cases you see:
	// - we are already the closest. Then all updates are relevant.
	// - we are becoming the closest. Then 'winner' changes, and we start updating towards the mesh.
	// - we are not the closest. Then we don't need to do anything beyond keeping track of our own distance to the trackable.
	//   (Which was already done before the ifstatement.)
	if (_winningReport.reporter == _myId) {
		LOGNearestCrownstoneTrackerVerbose("we already believed we were closest, so it's time to send an update towards the mesh");
		saveWinningReport(incomingReport, _myId);
		broadcastReport(incomingReport);
	}
	else {
		LOGNearestCrownstoneTrackerVerbose("we didn't win before");
		if (incomingReport.rssi > _winningReport.report.rssi)  {
			LOGNearestCrownstoneTrackerVerbose("but now we do, so have to send an update towards the mesh");
			saveWinningReport(incomingReport, _myId);
			broadcastReport(incomingReport);
			onWinnerChanged(true);
		}
		else {
			LOGNearestCrownstoneTrackerVerbose("we still don't, so we're done.");
		}
	}
}

void NearestCrownstoneTracker::onReceiveAssetReport(report_asset_id_t& incomingReport, stone_id_t reporter) {
	LOGNearestCrownstoneTrackerVerbose("onReceive witness report, myId(%u), reporter(%u), rssi(%i)", _myId, incomingReport.reporter, incomingReport.rssi);

	auto& _winningReport = getWinningReport(incomingReport.id);
	auto& _personalReport = getPersonalReport(incomingReport.id);


	if (reporter == _myId) {
		LOGNearestCrownstoneTrackerVerbose("Received an old report from myself. Dropped: not relevant.");
		return;
	}

	if (reporter == _winningReport.reporter) {
		LOGNearestCrownstoneTrackerVerbose("Received an update from the winner.");

		if (_personalReport.rssi > incomingReport.rssi) {
			LOGNearestCrownstoneTrackerVerbose("It dropped below my own value, so I win now. ");
			saveWinningReport(_personalReport, _myId);

			LOGNearestCrownstoneTrackerVerbose("Broadcast my personal report to update the mesh.");
			broadcastReport(_personalReport);

			onWinnerChanged(true);
		} else {
			LOGNearestCrownstoneTrackerVerbose("It still wins, so I'll just update the value of my winning report.");
			saveWinningReport(incomingReport, reporter);
		}
	}
	else {
		if (incomingReport.rssi > _winningReport.report.rssi) {
			LOGNearestCrownstoneTrackerVerbose("Received a witnessreport from another crownstone that is better than my winner.");
			saveWinningReport(incomingReport, reporter);
			onWinnerChanged(false);
		}
	}
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

internal_report_asset_id_t& NearestCrownstoneTracker::getWinningReport(short_asset_id_t& id) {
	return _winningReportAssetIds[0];  // TODO: find winning report for this id..
}

report_asset_id_t& NearestCrownstoneTracker::getPersonalReport(short_asset_id_t& id) {
	return _personalReportAssetIds[0];  // TODO: find winning report for this id..
}

// --------------------------- Report processing ------------------------


void NearestCrownstoneTracker::savePersonalReport(report_asset_id_t& report) {
	_personalReportAssetIds[0] = report;
}

void NearestCrownstoneTracker::saveWinningReport(report_asset_id_t& report, stone_id_t reporter) {
	_winningReportAssetIds[0] = {report, reporter};
}

void NearestCrownstoneTracker::resetReports() {
	_winningReportAssetIds[0] = {};
	_personalReportAssetIds[0] = {};

	_personalReportAssetIds[0].rssi = -127; // std::numeric_limits<int8_t>::lowest();
	_winningReportAssetIds[0].report.rssi = -127;
}

// ------------------- Mesh related stuff ----------------------

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
