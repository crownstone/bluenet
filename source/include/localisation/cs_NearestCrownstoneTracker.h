/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 17, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#ifndef SOURCE_INCLUDE_TRACKING_CS_NEARESTCROWNSTONE_H_
#define SOURCE_INCLUDE_TRACKING_CS_NEARESTCROWNSTONE_H_

#include <events/cs_EventListener.h>
#include <protocol/cs_Typedefs.h>

#include <localisation/cs_NearestWitnessReport.h>

class NearestCrownstoneTracker: public EventListener {
public:
	void init();

	void handleEvent(event_t &evt);

private:
	stone_id_t my_id; // cached for efficiency
	WitnessReport personal_report;
	WitnessReport winning_report;

	void onReceive(adv_background_parsed_t* trackable_advertisement);
	void onReceive(WitnessReport report);

	WitnessReport createReport(adv_background_parsed_t* trackable_advertisement);

	void savePersonalReport(WitnessReport report);
	void saveWinningReport(WitnessReport report);

	void broadcastReport(WitnessReport report);

	bool isValid(const WitnessReport& report); // crude implementation, only needed while not using maps for the reports

	void logReport(const char* text, WitnessReport report);

	/**
	 * Assumes my_id is set to the correct value.
	 */
	void resetReports();
};

#endif /* SOURCE_INCLUDE_TRACKING_CS_NEARESTCROWNSTONE_H_ */
