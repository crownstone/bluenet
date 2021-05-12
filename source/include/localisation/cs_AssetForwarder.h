/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 12, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <localisation/cs_AssetHandler.h>
#include <events/cs_EventListener.h>

class AssetForwarder : public EventListener, AssetHandlerMac {
public:
	cs_ret_code_t init();

	virtual void handleAcceptedAsset(AssetFilter f, const scanned_device_t& asset);

	virtual void handleEvent(event_t & event);

private:
	void forwardAssetToUart(const cs_mesh_model_msg_asset_rssi_mac_t& assetMsg);
};
