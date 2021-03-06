/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 20, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <localisation/cs_TrackableId.h>

/**
 * A Nearest Witness Report contains the rssi data and some meta-data
 * of a crownstone that received  an advertisement of a trackable device
 * without hops. (That crownstone 'witnessed' the trackable device.)
 *
 * These reports are communicated between crowntones in the mesh to determine
 * which crownstone is currently closest to the trackable device.
 *
 * The accuracy of the rssi data depends on physical conditions of the
 * witnessing crownstone.
 *
 * NearestWitnessReport is the firmware internal representation of witness
 * reports. This contains auxiliary data compared to the mesh representation
 * (rssi_data_message_t).
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
