/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 20, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <events/cs_Event.h>
#include <localisation/cs_TrackableId.h>

// REVIEW: Why a derivative of event_t, instead of a data struct?
// @bart: I think this makes the user code much cleaner.
/**
 * E.g.
 *
 * Now it's just:
 * 	TrackableEvent trackEvt;
 * 	trackEvt.rssi = scannedDevice->rssi;
 * 	trackEvt.dispatch();
 *
 * Rather than:
 * trackable_event_data_t trackdata;
 * trackdata.rssi = scannedDevice->rssi;
 * event_t trackevent(CS_TYPE::EVT_TRACKABLE, &trackdata, sizeof(trackdata);
 * trackevent.dispatch();
 *
 * Not only saving a line of non-trivial boilerplate at each call site, but also
 * reducing the surface at which we have to do size checks to the event class
 * implementations rather than al call sites.
 */
/**
 * Event that is emitted by TrackableParser when it receives advertisements
 * from trackable devices, after possible filtering and throttling.
 */
class TrackableEvent : public event_t {
public:
	TrackableId id;
	int8_t rssi;

	// (Meta data about how this trackable is throttled.)

	TrackableEvent();
};
