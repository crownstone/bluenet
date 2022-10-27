/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 20, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <util/cs_AssetFilter.h>

class AssetAcceptedEvent {
public:
	uint8_t _filterIndex;
	AssetFilter _primaryFilter;
	const scanned_device_t& _asset;

	AssetAcceptedEvent(uint8_t filterIndex, AssetFilter filter, const scanned_device_t& asset) : _filterIndex(filterIndex), _primaryFilter(filter), _asset(asset) {}
};
