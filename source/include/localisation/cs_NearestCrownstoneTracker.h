/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 17, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <events/cs_EventListener.h>
#include <localisation/cs_Nearestnearestwitnessreport.h>
#include <protocol/mesh/cs_MeshModelPackets.h>
#include <protocol/cs_Typedefs.h>

#include <util/cs_Coroutine.h>
#include <set>


class NearestCrownstoneTracker: public EventListener {
public:
	void init();

	void handleEvent(event_t &evt);

private:
	stone_id_t my_id; // cached for efficiency
	NearestWitnessReport personal_report;
	NearestWitnessReport winning_report;

	void onReceive(adv_background_parsed_t* trackable_advertisement);
	void onReceive(NearestWitnessReport& report);

	NearestWitnessReport createReport(adv_background_parsed_t* trackable_advertisement);
	NearestWitnessReport createReport(MeshMsgEvent* trackable_advertisement);

	nearest_witness_report_t reduceReport(const NearestWitnessReport& report);

	void savePersonalReport(NearestWitnessReport report);
	void saveWinningReport(NearestWitnessReport report);

	void broadcastReport(NearestWitnessReport report);
	void onWinnerChanged();

	bool isValid(const NearestWitnessReport& report); // crude implementation, only needed while not using maps for the reports

	void logReport(const char* text, NearestWitnessReport report);

	/**
	 * Assumes my_id is set to the correct value.
	 */
	void resetReports();


	// DEBUG
	void onReceive(scanned_device_t scanned_device);
	std::set<TrackableId> received_uuids = {};
	Coroutine uuid_printer;
	// END DEBUG
};

