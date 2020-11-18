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

	personal_report.reporter = my_id;
	personal_report.rssi = -127; // std::numeric_limits<uint8_t>::lowest();
	personal_report.trackable = { 0 };

	winning_report = { 0 };
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

NearestCrownstoneTracker::SquashedMacAddress NearestCrownstoneTracker::getSquashed(
        uint8_t *mac) {
	SquashedMacAddress squash;
	squash.bytes[0] = mac[0] ^ mac[1];
	squash.bytes[1] = mac[2] ^ mac[3];
	squash.bytes[2] = mac[4] ^ mac[5];
	return squash;
}

NearestCrownstoneTracker::WitnessReport NearestCrownstoneTracker::createReport(
        adv_background_parsed_t *trackable_advertisement) {
	WitnessReport report;
	report.reporter = my_id;
	report.trackable = getSquashed(trackable_advertisement->macAddress);
	report.rssi = trackable_advertisement->adjustedRssi;
	return report;
}

void NearestCrownstoneTracker::savePersonalReport(WitnessReport report) {
	logReport("saving personal report", report);
//	personal_report = report;
}

void NearestCrownstoneTracker::saveWinningReport(WitnessReport report) {
	logReport("saving winning report", report);
//	winning_report = report;
}

void NearestCrownstoneTracker::broadcastReport(WitnessReport report) {
	// TODO
	logReport("broadcasting report", report);
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
