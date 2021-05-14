/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 11, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <localisation/cs_AssetFilterPacketAccessors.h>
#include <structs/cs_PacketsInternal.h>


class AssetHandlerMac {
public:
	/**
	 * To be called when the AssetFiltering component has accepted an asset that can
	 * be identified by a short asset id.
	 */
	virtual void handleAcceptedAsset(AssetFilter f, const scanned_device_t& asset) = 0;
	virtual ~AssetHandlerMac() = default;
};

class AssetHandlerShortId {
public:
	/**
	 * To be called when the AssetFiltering component has accepted an asset that can
	 * be identified by its mac address.
	 */
	virtual void handleAcceptedAsset(AssetFilter f, const scanned_device_t& asset, short_asset_id_t assetId) = 0;
	virtual ~AssetHandlerShortId() = default;
};
