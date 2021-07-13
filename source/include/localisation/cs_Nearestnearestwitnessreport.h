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
 * reports. This contains auxiliary data compared to the mesh representation.
 */
struct __attribute__((__packed__)) internal_report_asset_id_t {
	report_asset_id_t report;
	stone_id_t reporter;
};

struct __attribute__((__packed__)) internal_report_asset_mac_t {
	report_asset_mac_t report;
	stone_id_t reporter;
};
