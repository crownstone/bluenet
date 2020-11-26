/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 17, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <localisation/cs_NearestCrownstoneTracker.h>

#include <common/cs_Types.h>
#include <drivers/cs_Serial.h>
#include <events/cs_Event.h>
#include <protocol/mesh/cs_MeshModelPackets.h>
#include <storage/cs_State.h>

#define LOGNearestCrownstoneTrackerVerbose LOGnone
#define LOGNearestCrownstoneTrackerDebug LOGnone
#define LOGNearestCrownstoneTrackerInfo LOGi

void NearestCrownstoneTracker::init() {
	State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &my_id,
	        sizeof(my_id));

	resetReports();

	logReport("init report: ", personal_report);
}

void NearestCrownstoneTracker::handleEvent(event_t &evt) {
	if (evt.type == CS_TYPE::EVT_ADV_BACKGROUND_PARSED) {
		adv_background_parsed_t *parsed_adv =
		        reinterpret_cast<TYPIFY(EVT_ADV_BACKGROUND_PARSED)*>(evt.data);
		onReceive(parsed_adv);
	}

	if(evt.type == CS_TYPE::EVT_MESH_NEAREST_WITNESS_REPORT) {
		LOGNearestCrownstoneTrackerVerbose("NearestCrownstone received event: EVT_MESH_NEAREST_WITNESS_REPORT");
		MeshMsgEvent* mesh_msg_event =
				reinterpret_cast<TYPIFY(EVT_MESH_NEAREST_WITNESS_REPORT)*>(evt.data);
		NearestWitnessReport report = createReport(mesh_msg_event);
		onReceive(report);
	}
}

void NearestCrownstoneTracker::onReceive(
        adv_background_parsed_t *trackable_advertisement) {
	LOGNearestCrownstoneTrackerVerbose("onReceive trackable, my_id(%d)", my_id);
	auto incoming_report = createReport(trackable_advertisement);

	logReport("incoming report", incoming_report);

	savePersonalReport(incoming_report);

	// TODO: replace condition with std::find when
	// we have a map of uuid --> report for all the datas

	if (isValid(winning_report)) {
		if(winning_report.reporter == my_id){
			LOGNearestCrownstoneTrackerVerbose("we already believed we were closest, so this is just an update");
			saveWinningReport(incoming_report);
			broadcastReport(incoming_report);
		} else {
			LOGNearestCrownstoneTrackerVerbose("we didn't win before");
			if (incoming_report.rssi > winning_report.rssi)  {
				LOGNearestCrownstoneTrackerVerbose("but now we do, so have to do something");
				saveWinningReport(incoming_report);
				broadcastReport(incoming_report);
				onWinnerChanged();
			} else {
				LOGNearestCrownstoneTrackerVerbose("we still don't, so we're done.");
			}
		}
	} else {
		LOGNearestCrownstoneTrackerVerbose("no winning report yet, so our personal one wins");
		saveWinningReport(incoming_report);
		broadcastReport(incoming_report);
		onWinnerChanged();
	}
}

void NearestCrownstoneTracker::onReceive(NearestWitnessReport& incoming_report) {
	LOGNearestCrownstoneTrackerVerbose("onReceive witness report, my_id(%d), reporter(%d), rssi(%d)", my_id, incoming_report.reporter, incoming_report.rssi);
	logReport("", incoming_report);

	if(incoming_report.reporter == my_id) {
		LOGNearestCrownstoneTrackerVerbose("Received an old report from myself. Dropped: not relevant.");
		return;
	}

	if(!isValid(winning_report)) {
		LOGNearestCrownstoneTrackerVerbose("Didn't have a incoming_report yet. Updated my winner.");
		saveWinningReport(incoming_report);
		onWinnerChanged();
	} else if(incoming_report.reporter == winning_report.reporter) {
		LOGNearestCrownstoneTrackerVerbose("Received an update from the winner.");

		if(personal_report.rssi > incoming_report.rssi){
			LOGNearestCrownstoneTrackerVerbose("It dropped below my own value, so I win now. ");
			saveWinningReport(personal_report);

			LOGNearestCrownstoneTrackerVerbose("Broadcast my personal report to update the mesh.");
			broadcastReport(personal_report);

			onWinnerChanged();
		} else {
			LOGNearestCrownstoneTrackerVerbose("It still wins, so I'll just update the value of my winning report.");
			saveWinningReport(incoming_report);
		}
	} else {
		if(incoming_report.rssi > winning_report.rssi) {
			LOGNearestCrownstoneTrackerVerbose("Received a witnessreport from another crownstone that is better than my winner.");
			saveWinningReport(incoming_report);
			onWinnerChanged();
		}
	}
}

void NearestCrownstoneTracker::onWinnerChanged() {
	LOGNearestCrownstoneTrackerInfo("Winner changed. Turning %s",winning_report.reporter == my_id ? "on":"off");

	CS_TYPE on_off =
			winning_report.reporter == my_id
			? CS_TYPE::CMD_SWITCH_ON
			: CS_TYPE::CMD_SWITCH_OFF;

	event_t event(on_off, nullptr, 0, cmd_source_t(CS_CMD_SOURCE_SWITCHCRAFT));
	event.dispatch();
}

// --------------------------- Report processing ------------------------

NearestWitnessReport NearestCrownstoneTracker::createReport(
        adv_background_parsed_t *trackable_advertisement) {
	NearestWitnessReport report;
	report.reporter = my_id;
	report.trackable = Uuid(trackable_advertisement->macAddress);
	report.rssi = trackable_advertisement->adjustedRssi;
	return report;
}

NearestWitnessReport NearestCrownstoneTracker::createReport(MeshMsgEvent* mesh_msg_event) {
	auto nearest_witness_report = mesh_msg_event->getPacket<CS_MESH_MODEL_TYPE_NEAREST_WITNESS_REPORT>();

	return NearestWitnessReport(
			nearest_witness_report.trackable_device_mac,
	        nearest_witness_report.rssi,
	        mesh_msg_event->srcAddress);
}

nearest_witness_report_t NearestCrownstoneTracker::reduceReport(const NearestWitnessReport& report) {
	nearest_witness_report_t reduced_report;
	std::memcpy(
			reduced_report.trackable_device_mac,
			report.trackable.bytes,
			sizeof(reduced_report.trackable_device_mac)
	);

	reduced_report.rssi = report.rssi;

	return reduced_report;
}

void NearestCrownstoneTracker::savePersonalReport(NearestWitnessReport report) {
	personal_report = report;
	logReport("saved personal report", report);
}

void NearestCrownstoneTracker::saveWinningReport(NearestWitnessReport report) {
	winning_report = report;
	logReport("saved winning report", winning_report);
}


bool NearestCrownstoneTracker::isValid(const NearestWitnessReport& report) {
	return report.rssi != 0;
}

void NearestCrownstoneTracker::logReport(const char* text, NearestWitnessReport report) {
	LOGNearestCrownstoneTrackerDebug("%s {reporter:%d, trackable: %x %x %x %x %x %x, rssi:%d dB}",
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
	winning_report = NearestWitnessReport();
	personal_report = NearestWitnessReport();

	personal_report.reporter = my_id;
	personal_report.rssi = -127; // std::numeric_limits<uint8_t>::lowest();
}

// ------------------- Mesh related stuff ----------------------

void NearestCrownstoneTracker::broadcastReport(NearestWitnessReport report) {
	logReport("broadcasting report", report);
	nearest_witness_report_t packed_report = reduceReport(report);

	cs_mesh_msg_t report_msg_wrapper;
	report_msg_wrapper.type =  CS_MESH_MODEL_TYPE_NEAREST_WITNESS_REPORT;
	report_msg_wrapper.payload = reinterpret_cast<uint8_t*>(&packed_report);
	report_msg_wrapper.size = sizeof(packed_report);
	report_msg_wrapper.reliability = CS_MESH_RELIABILITY_LOW;
	report_msg_wrapper.urgency = CS_MESH_URGENCY_LOW;

	event_t report_msg_evt(CS_TYPE::CMD_SEND_MESH_MSG, &report_msg_wrapper,
	        sizeof(cs_mesh_msg_t));

	report_msg_evt.dispatch();

}
