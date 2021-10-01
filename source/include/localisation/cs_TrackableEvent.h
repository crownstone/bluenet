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
	AssetFilter _primaryFilter;
	const scanned_device_t& _asset;
	uint8_t _acceptedFilterIdBitmask;

	AssetAcceptedEvent(AssetFilter filter, const scanned_device_t& asset, uint8_t acceptedFilterIdBitmask)
		: _primaryFilter(filter), _asset(asset), _acceptedFilterIdBitmask(acceptedFilterIdBitmask) {}
};

