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
#include <storage/cs_State.h>


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
}


void NearestCrownstoneTracker::onReceive(
        adv_background_parsed_t *trackable_advertisement) {
	LOGi("onReceive trackable, my_id(%d)", my_id);
	auto incoming_report = createReport(trackable_advertisement);

	logReport("incoming report", incoming_report);

	savePersonalReport(incoming_report);

	// TODO: replace condition with std::find when
	// we have a map of uuid --> report for all the datas

	if (isValid(winning_report)) {
		if(winning_report.reporter == my_id){
			LOGi("we already believed we were closest, so this is just an update");
			saveWinningReport(incoming_report);
			broadcastReport(incoming_report);
		} else {
			LOGi("we didn't win before");
			if (incoming_report.rssi > winning_report.rssi)  {
				LOGi("but now we do, so have to do something");
				saveWinningReport(incoming_report);
				broadcastReport(incoming_report);
			} else {
				LOGi("we still don't, so we're done.");
			}
		}
	} else {
		LOGi("no winning report yet, so our personal one wins");
		saveWinningReport(incoming_report);
		broadcastReport(incoming_report);
	}
	LOGi("----");
}

void NearestCrownstoneTracker::onReceive(WitnessReport report) {
	// TODO
}

// --------------------------- Report processing ------------------------

WitnessReport NearestCrownstoneTracker::createReport(
        adv_background_parsed_t *trackable_advertisement) {
	WitnessReport report;
	report.reporter = my_id;
	report.trackable = SquashedMacAddress(trackable_advertisement->macAddress);
	report.rssi = trackable_advertisement->adjustedRssi;
	return report;
}

void NearestCrownstoneTracker::savePersonalReport(WitnessReport report) {
	personal_report = report;
	logReport("saved personal report", report);
}

void NearestCrownstoneTracker::saveWinningReport(WitnessReport report) {
	winning_report = report;
	logReport("saved winning report", winning_report);
}


bool NearestCrownstoneTracker::isValid(const WitnessReport& report) {
	return report.rssi != 0;
}

void NearestCrownstoneTracker::logReport(const char* text, WitnessReport report) {
	LOGi("%s {reporter:%d, trackable: %x %x %x, rssi:%d dB}",
			text,
			report.reporter,
			report.trackable.bytes[0],
			report.trackable.bytes[1],
			report.trackable.bytes[2],
			report.rssi
	);
}

void NearestCrownstoneTracker::resetReports() {
	winning_report = WitnessReport();
	personal_report = WitnessReport();

	personal_report.reporter = my_id;
	personal_report.rssi = -127; // std::numeric_limits<uint8_t>::lowest();
}

// ------------------- Mesh related stuff ----------------------

void NearestCrownstoneTracker::broadcastReport(WitnessReport report) {
	// TODO
	logReport("broadcasting report", report);

//	cs_mesh_msg_t report_msg_wrapper;
//	report_msg_wrapper.type =  ;//CS_MESH_MODEL_TYPE_RSSI_PING;
//	report_msg_wrapper.payload = reinterpret_cast<uint8_t*>(&report);
//	report_msg_wrapper.size = ;//sizeof(rssi_ping_message_t);
//	report_msg_wrapper.reliability = CS_MESH_RELIABILITY_LOW;
//	report_msg_wrapper.urgency = CS_MESH_URGENCY_LOW;
//
//	event_t report_msg_evt(CS_TYPE::CMD_SEND_MESH_MSG, &report_msg_wrapper,
//	        sizeof(cs_mesh_msg_t));
//
//	report_msg_evt.dispatch();

}
