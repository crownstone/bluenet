/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 17, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#ifndef SOURCE_INCLUDE_TRACKING_CS_NEARESTCROWNSTONE_H_
#define SOURCE_INCLUDE_TRACKING_CS_NEARESTCROWNSTONE_H_

#include <events/cs_EventListener.h>
#include <localisation/cs_Nearestnearestwitnessreportt.h>
#include <protocol/cs_Typedefs.h>


class NearestCrownstoneTracker: public EventListener {
public:
	void init();

	void handleEvent(event_t &evt);

private:
	stone_id_t my_id; // cached for efficiency
	nearest_witness_report_t personal_report;
	nearest_witness_report_t winning_report;

	void onReceive(adv_background_parsed_t* trackable_advertisement);
	void onReceive(nearest_witness_report_t report);

	nearest_witness_report_t createReport(adv_background_parsed_t* trackable_advertisement);

	void savePersonalReport(nearest_witness_report_t report);
	void saveWinningReport(nearest_witness_report_t report);

	void broadcastReport(nearest_witness_report_t report);

	bool isValid(const nearest_witness_report_t& report); // crude implementation, only needed while not using maps for the reports

	void logReport(const char* text, nearest_witness_report_t report);

	/**
	 * Assumes my_id is set to the correct value.
	 */
	void resetReports();
};

#endif /* SOURCE_INCLUDE_TRACKING_CS_NEARESTCROWNSTONE_H_ */
