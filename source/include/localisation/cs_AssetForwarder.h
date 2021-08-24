/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 12, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <common/cs_Component.h>
#include <events/cs_EventListener.h>

#include <localisation/cs_AssetHandler.h>

class AssetForwarder : public EventListener, public Component {
	static constexpr uint16_t MIN_THROTTLED_ADVERTISEMENT_PERIOD_MS = 1000;

public:
	cs_ret_code_t init();

	/**
	 * Forward asset as CS_MESH_MODEL_TYPE_ASSET_RSSI_MAC to the mesh
	 *
	 * returns desired minimal time to next advertisement.
	 */
	uint16_t handleAcceptedAsset(const scanned_device_t& asset);

	/**
	 * Forward asset as CS_MESH_MODEL_TYPE_ASSET_RSSI_SID to the mesh
	 *
	 * returns desired minimal time to next advertisement.
	 * // TODO: change to 'record Updated'?
	 */
	uint16_t handleAcceptedAsset(const scanned_device_t& asset, const short_asset_id_t& sid);

	virtual void handleEvent(event_t & event);

private:
	stone_id_t _myId;

	void forwardAssetToUart(const cs_mesh_model_msg_asset_rssi_mac_t& assetMsg, stone_id_t sender);
	void forwardAssetToUart(const cs_mesh_model_msg_asset_rssi_sid_t& assetMsg, stone_id_t sender);

	cs_asset_rssi_data_mac_t constructUartMsg(const cs_mesh_model_msg_asset_rssi_mac_t& assetMsg,
			 stone_id_t sender);
	cs_asset_rssi_data_sid_t constructUartMsg(const cs_mesh_model_msg_asset_rssi_sid_t& assetMsg,
				 stone_id_t sender);
};
