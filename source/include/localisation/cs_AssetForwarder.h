/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 12, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <localisation/cs_AssetHandler.h>
#include <events/cs_EventListener.h>

class AssetForwarder : public EventListener {
public:
	cs_ret_code_t init();

	void handleAcceptedAsset(const scanned_device_t& asset);

	virtual void handleEvent(event_t & event);

private:
	stone_id_t _myId;

	void forwardAssetToUart(const cs_mesh_model_msg_asset_rssi_mac_t& assetMsg, stone_id_t sender);
	cs_asset_rssi_data_t constructUartMsg(const cs_mesh_model_msg_asset_rssi_mac_t& assetMsg,
			 stone_id_t sender);
};
