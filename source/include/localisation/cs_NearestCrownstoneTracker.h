/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 17, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <events/cs_EventListener.h>
#include <localisation/cs_Nearestnearestwitnessreport.h>
#include <localisation/cs_TrackableEvent.h>
#include <protocol/mesh/cs_MeshModelPackets.h>
#include <protocol/cs_Typedefs.h>

// REVIEW: Missing description.
class NearestCrownstoneTracker: public EventListener {
public:
	/**
	 * Caches CONFIG_CROWNSTONE_ID, and resets the report maps.
	 */
	void init();

	/**
	 * Handlers for:
	 * EVT_MESH_NEAREST_WITNESS_REPORT
	 * EVT_TRACKABLE
	 */
	void handleEvent(event_t &evt);

private:
	stone_id_t my_id; // cached for efficiency

	// REVIEW: What is winning, why not call it nearest? What is personal?
	// TODO: expand to a map<TrackableID, NearestWitnessReport>.
	NearestWitnessReport personal_report;
	NearestWitnessReport winning_report;

	/**
	 * Heart of the algorithm. See implementation for exact behaviour.
	 *
	 * Updates personal report, possibly updates winning report
	 * and possibly broadcasts a message to inform other devices
	 * in the mesh of relevant changes.
	 */
	void onReceive(TrackableEvent* tracked_event);
	/**
	 * Heart of the algorithm. See implementation for exact behaviour.
	 *
	 * Possibly updates winning report and possibly broadcasts
	 * a message to inform other devices in the mesh of relevant changes.
	 * E.g. when the updated winning report now loses from this devices
	 * personal report.
	 */
	void onReceive(NearestWitnessReport& report);

	NearestWitnessReport createReport(TrackableEvent* tracked_event);
	NearestWitnessReport createReport(MeshMsgEvent* trackable_advertisement);

	/**
	 * Returns a nearest_witness_report_t that can be sent over the mesh.
	 */
	nearest_witness_report_t reduceReport(const NearestWitnessReport& report);

	/**
	 * Only does an asignment to personal_report.
	 */
	void savePersonalReport(NearestWitnessReport report);

	/**
	 * Only does an asignment to winning_report.
	 */
	void saveWinningReport(NearestWitnessReport report);

	/**
	 * Sends a mesh broadcast for the given report.
	 */
	void broadcastReport(NearestWitnessReport report);

	/**
	 * Currently dispatches an event for CMD_SWITCH_[ON,OFF] depending on
	 * whether this crownstone is closest (ON) or not the closest.
	 *
	 * Also prints a debug log stating the current winner. // REVIEW: Implementation detail.
	 */
	void onWinnerChanged();

	// REVIEW: Describes implementation instead of what it does/is for.
	/**
	 * Returns report.rssi != 0.
	 *
	 * Note: when adding the multiple TrackableIds feature this method
	 * becomes obsolete. We can then use map::find to check if a winnin/personal
	 * report exists/isvalid.
	 */
	bool isValid(const NearestWitnessReport& report);

	/**
	 * Assumes my_id is set to the stone id of this crownstone.
	 */
	void resetReports();

	void logReport(const char* text, NearestWitnessReport report);
};

