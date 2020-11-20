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

#include <cstring> // for memcmp

class NearestCrownstoneTracker: public EventListener {
public:
	void init();

	void handleEvent(event_t &evt);

private:
	/**
	 * Shortcut for MVP implementation:
	 * - We may encounter mac addresses with the same squashed address
	 * - Squashing is not invertible
	 *
	 * Better alternative:
	 * - upgrade tracked device tokens to work for any mac? (And they do rotations.. yay)
	 */
	struct SquashedMacAddress {
		uint8_t bytes[3];

		SquashedMacAddress() = default;

		SquashedMacAddress(const SquashedMacAddress& other){
			std::memcpy(bytes, other.bytes, 3);
		}

		bool operator==(const SquashedMacAddress& other){
			return std::memcmp(bytes,other.bytes,3) == 0;
		}
	};

	/**
	 * bytes = [mac[2*i] ^ mac[2*i+1] for i in range(3)]
	 */
	SquashedMacAddress getSquashed(uint8_t *mac);

	struct WitnessReport {
		SquashedMacAddress trackable;
		int8_t rssi;
		stone_id_t reporter;

		WitnessReport(WitnessReport &other) :
				trackable(other.trackable),
				rssi(other.rssi),
				reporter(other.reporter) {
		}

		WitnessReport() = default;
	};

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
