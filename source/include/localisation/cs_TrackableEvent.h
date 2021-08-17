/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 20, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <localisation/cs_AssetFilterPacketAccessors.h>

class AssetAcceptedEvent {
public:
	AssetFilter _primaryFilter;
	const scanned_device_t& _asset;
	uint8_t _acceptedFilterIdBitmask;

	AssetAcceptedEvent(AssetFilter filter, const scanned_device_t& asset, uint8_t acceptedFilterIdBitmask)
		: _primaryFilter(filter), _asset(asset), _acceptedFilterIdBitmask(acceptedFilterIdBitmask) {}
};

class AssetWithSidAcceptedEvent {
public:
	AssetFilter _primaryFilter;

	const scanned_device_t& _asset;
	short_asset_id_t _id;
	uint8_t _acceptedFilterIdBitmask;

	AssetWithSidAcceptedEvent(
			AssetFilter filter, const scanned_device_t& asset, short_asset_id_t id, uint8_t acceptedFilterIdBitmask)
		: _primaryFilter(filter), _asset(asset), _id(id), _acceptedFilterIdBitmask(acceptedFilterIdBitmask) {}
};
