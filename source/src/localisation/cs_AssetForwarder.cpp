/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 12, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <localisation/cs_AssetForwarder.h>
#include <events/cs_EventListener.h>
#include <util/cs_Rssi.h>
#include <logging/cs_Logger.h>

#define LOGAssetForwarderDebug LOGd


void AssetForwarder::handleAcceptedAsset(AssetFilter f, const scanned_device_t& asset) {
	cs_mesh_model_msg_asset_rssi_mac_t asset_msg;
	asset_msg.rssiData = compressRssi(asset.rssi, asset.channel);
	memcpy(asset_msg.mac, asset.address, sizeof(asset_msg.mac));
	LOGAssetForwarderDebug("AssetFiltering::dispatchAcceptedAssetMacToMesh: ch%u @ -%u dB",
			asset_msg.rssiData.channel + 36,
			asset_msg.rssiData.rssi_halved * 2);

	cs_mesh_msg_t msgWrapper;
	msgWrapper.type        = CS_MESH_MODEL_TYPE_ASSET_RSSI_MAC;
	msgWrapper.payload     = reinterpret_cast<uint8_t*>(&asset_msg);
	msgWrapper.size        = sizeof(asset_msg);
	msgWrapper.reliability = CS_MESH_RELIABILITY_LOW;
	msgWrapper.urgency     = CS_MESH_URGENCY_LOW;

	event_t meshMsgEvt(CS_TYPE::CMD_SEND_MESH_MSG, &msgWrapper, sizeof(msgWrapper));

	meshMsgEvt.dispatch();
}
