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
#include <protocol/mesh/cs_MeshModelPackets.h>

class AssetForwarder : public EventListener, public Component {
	static constexpr uint16_t MIN_THROTTLED_ADVERTISEMENT_PERIOD_MS = 1000;

public:
	cs_ret_code_t init();

	/**
	 * Forward asset as CS_MESH_MODEL_TYPE_ASSET_RSSI_MAC to the mesh
	 *
	 * returns desired minimal time to next advertisement.
	 */
	uint16_t sendAssetMacToMesh(const scanned_device_t& asset);

	/**
	 * Forward asset as CS_MESH_MODEL_TYPE_ASSET_INFO to the mesh
	 *
	 * returns desired minimal time to next advertisement.
	 */
	uint16_t sendAssetIdToMesh(const scanned_device_t& asset, const asset_id_t& assetId, uint8_t filterBitmask);

	virtual void handleEvent(event_t & event);

private:
	stone_id_t _myStoneId;

	/**
	 * Forward an asset mesh message to UART.
	 *
	 * @param[in] assetMsg             The mesh message to forward.
	 * @param[in] seenByStoneId        The stone that scanned the asset.
	 */
	void forwardAssetToUart(const cs_mesh_model_msg_asset_info_mac_t& assetMsg, stone_id_t seenByStoneId);
	void forwardAssetToUart(const cs_mesh_model_msg_asset_info_id_t& assetMsg, stone_id_t seenByStoneId);
};
