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
	if (evt.type == CS_TYPE::EVT_MESH_NEAREST_WITNESS_REPORT) {
		// another crownstone has reported information.
		LOGNearestCrownstoneTrackerVerbose("NearestCrownstone received event: EVT_MESH_NEAREST_WITNESS_REPORT");
		MeshMsgEvent* meshMsgEvent = CS_TYPE_CAST(EVT_MESH_NEAREST_WITNESS_REPORT, evt.data);
		NearestWitnessReport report = createReport(meshMsgEvent);
		onReceive(report);
		return;
	}

	if (evt.type == CS_TYPE::EVT_TRACKABLE) {
		// a trackable asset is witnessed by this crownstone.
		TrackableEvent* trackEvt = CS_TYPE_CAST(EVT_TRACKABLE, evt.data);
		onReceive(trackEvt);
	}

	if (evt.type == CS_TYPE::EVT_ASSET_ACCEPTED) {
		AssetAcceptedEvent* assetAcceptedEvent = CS_TYPE_CAST(EVT_ASSET_ACCEPTED, evt.data);
		// TODO(Arend)
	}
}


void NearestCrownstoneTracker::onReceive(TrackableEvent* trackedEvent) {
	LOGNearestCrownstoneTrackerVerbose("onReceive trackable, myId(%u)", _myId);
	auto incomingReport = createReport(trackedEvent);

	savePersonalReport(incomingReport);

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
		saveWinningReport(incomingReport);
		broadcastReport(incomingReport);
	}
	else {
		LOGNearestCrownstoneTrackerVerbose("we didn't win before");
		if (incomingReport.rssi > _winningReport.rssi)  {
			LOGNearestCrownstoneTrackerVerbose("but now we do, so have to send an update towards the mesh");
			saveWinningReport(incomingReport);
			broadcastReport(incomingReport);
			onWinnerChanged();
		}
		else {
			LOGNearestCrownstoneTrackerVerbose("we still don't, so we're done.");
		}
	}
}

void NearestCrownstoneTracker::onReceive(NearestWitnessReport& incomingReport) {
	LOGNearestCrownstoneTrackerVerbose("onReceive witness report, myId(%u), reporter(%u), rssi(%i)", _myId, incomingReport.reporter, incomingReport.rssi);

	if (incomingReport.reporter == _myId) {
		LOGNearestCrownstoneTrackerVerbose("Received an old report from myself. Dropped: not relevant.");
		return;
	}

	if (incomingReport.reporter == _winningReport.reporter) {
		LOGNearestCrownstoneTrackerVerbose("Received an update from the winner.");

		if (_personalReport.rssi > incomingReport.rssi) {
			LOGNearestCrownstoneTrackerVerbose("It dropped below my own value, so I win now. ");
			saveWinningReport(_personalReport);

			LOGNearestCrownstoneTrackerVerbose("Broadcast my personal report to update the mesh.");
			broadcastReport(_personalReport);

			onWinnerChanged();
		} else {
			LOGNearestCrownstoneTrackerVerbose("It still wins, so I'll just update the value of my winning report.");
			saveWinningReport(incomingReport);
		}
	}
	else {
		if (incomingReport.rssi > _winningReport.rssi) {
			LOGNearestCrownstoneTrackerVerbose("Received a witnessreport from another crownstone that is better than my winner.");
			saveWinningReport(incomingReport);
			onWinnerChanged();
		}
	}
}

void NearestCrownstoneTracker::onWinnerChanged() {
	LOGd("Nearest changed to stoneid=%u. I'm turning %s",
			_winningReport.reporter,
			_winningReport.reporter == _myId ? "on":"off");

	CS_TYPE onOff =
			_winningReport.reporter == _myId
			? CS_TYPE::CMD_SWITCH_ON
			: CS_TYPE::CMD_SWITCH_OFF;

	event_t event(onOff, nullptr, 0, cmd_source_t(CS_CMD_SOURCE_SWITCHCRAFT));
	event.dispatch();
}

// --------------------------- Report processing ------------------------

NearestWitnessReport NearestCrownstoneTracker::createReport(TrackableEvent* trackedEvent) {
	return NearestWitnessReport(trackedEvent->id, trackedEvent->rssi, _myId);
}

// REVIEW: Return by ref?
// @Bart: RVO (return value optimization) is guaranteed to take place
// as the constructor call is part of the return statement.
// (This means the return value is constructed on the stack frame below the current one.)
NearestWitnessReport NearestCrownstoneTracker::createReport(MeshMsgEvent* meshMsgEvent) {
	// REVIEW: Lot of mesh implementation details: shouldn't this be done in the mesh msg handler?
	// @Bart: We cannot. That would imply the component gets severely intertwined with other components
	// and makes modularization a mess.
	auto nearestWitnessReport = meshMsgEvent->getPacket<CS_MESH_MODEL_TYPE_REPORT_ASSET_MAC>();

	return NearestWitnessReport(
			TrackableId(
					nearestWitnessReport.trackableDeviceMac,
					sizeof(nearestWitnessReport.trackableDeviceMac)),
			nearestWitnessReport.rssi,
			meshMsgEvent->srcAddress);
}

report_asset_mac_t NearestCrownstoneTracker::reduceReport(const NearestWitnessReport& report) {
	report_asset_mac_t reducedReport;
	std::memcpy(
			reducedReport.trackableDeviceMac,
			report.trackable.bytes,
			sizeof(reducedReport.trackableDeviceMac)
	);

	reducedReport.rssi = report.rssi;

	return reducedReport;
}

void NearestCrownstoneTracker::savePersonalReport(NearestWitnessReport report) {
	_personalReport = report;
}

void NearestCrownstoneTracker::saveWinningReport(NearestWitnessReport report) {
	_winningReport = report;
}

void NearestCrownstoneTracker::logReport(const char* text, NearestWitnessReport report) {
	LOGNearestCrownstoneTrackerDebug("%s {reporter:%u, trackable: %x %x %x %x %x %x, rssi:%i dB}",
			text,
			report.reporter,
			report.trackable.bytes[0],
			report.trackable.bytes[1],
			report.trackable.bytes[2],
			report.trackable.bytes[3],
			report.trackable.bytes[4],
			report.trackable.bytes[5],
			report.rssi
	);
}

void NearestCrownstoneTracker::resetReports() {
	_winningReport = NearestWitnessReport();
	_personalReport = NearestWitnessReport();

	_personalReport.reporter = _myId;
	_personalReport.rssi = -127; // std::numeric_limits<int8_t>::lowest();
	_winningReport.rssi = -127;
}

// ------------------- Mesh related stuff ----------------------

void NearestCrownstoneTracker::broadcastReport(NearestWitnessReport report) {
	report_asset_mac_t packedReport = reduceReport(report);

	cs_mesh_msg_t reportMsgWrapper;
	reportMsgWrapper.type =  CS_MESH_MODEL_TYPE_REPORT_ASSET_MAC;
	reportMsgWrapper.payload = reinterpret_cast<uint8_t*>(&packedReport);
	reportMsgWrapper.size = sizeof(packedReport);
	reportMsgWrapper.reliability = CS_MESH_RELIABILITY_LOW;
	reportMsgWrapper.urgency = CS_MESH_URGENCY_LOW;

	event_t reportMsgEvt(CS_TYPE::CMD_SEND_MESH_MSG, &reportMsgWrapper, sizeof(reportMsgWrapper));

	reportMsgEvt.dispatch();

}
