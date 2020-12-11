/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 20, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <localisation/cs_TrackableId.h>
#include <events/cs_Event.h>

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
