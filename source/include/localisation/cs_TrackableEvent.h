/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 20, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <localisation/cs_TrackableId.h>
#include <localisation/cs_AssetFilterPacketAccessors.h>

class TrackableEvent {
public:
	TrackableId id;
	int8_t rssi;
};


class AssetAcceptedEvent {
public:
	AssetFilter _filter;
	const scanned_device_t& _asset;

	AssetAcceptedEvent(AssetFilter filter, const scanned_device_t& asset) :
		_filter(filter), _asset(asset) {}
};
