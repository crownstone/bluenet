/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 20, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <localisation/cs_TrackableId.h>

// REVIEW: Someone reading this doesn't know what a witness report is.
/**
 * Firmware internal representation of witness reports.
 * This contains auxiliary data compared to the mesh representation.
 */
class NearestWitnessReport {
public:
	TrackableId trackable;
	int8_t rssi = 0;
	stone_id_t reporter = 0;

	/**
	 * copy constructor enables assignment.
	 */
	NearestWitnessReport(NearestWitnessReport &other) :
			trackable(other.trackable),
			rssi(other.rssi),
			reporter(other.reporter) {
	}

	NearestWitnessReport(TrackableId mac, int8_t rssi, stone_id_t id) :
			trackable(mac), rssi(rssi), reporter(id) {
	}

	NearestWitnessReport() = default;
};
