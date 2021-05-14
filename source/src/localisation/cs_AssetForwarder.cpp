/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 12, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <localisation/cs_AssetForwarder.h>
#include <mesh/cs_MeshMsgEvent.h>

#include <util/cs_Rssi.h>
#include <protocol/cs_Packets.h>
#include <logging/cs_Logger.h>

#define LOGAssetForwarderDebug LOGd

void printAsset(const cs_mesh_model_msg_asset_rssi_mac_t& assetMsg) {
	LOGAssetForwarderDebug("AssetFiltering::dispatchAcceptedAssetMacToMesh: ch%u @ -%u dB",
			getChannel(assetMsg.rssiData),
			getRssiUnsigned(assetMsg.rssiData));
}


cs_ret_code_t AssetForwarder::init() {
	listen();
	return ERR_SUCCESS;
}

void AssetForwarder::handleAcceptedAsset(AssetFilter f, const scanned_device_t& asset) {
	cs_mesh_model_msg_asset_rssi_mac_t asset_msg;
	asset_msg.rssiData = compressRssi(asset.rssi, asset.channel);
	memcpy(asset_msg.mac, asset.address, sizeof(asset_msg.mac));
	LOGAssetForwarderDebug("handledAcceptedAsset ch%u, %d dB", asset.channel, asset.rssi);
	printAsset(asset_msg);

	cs_mesh_msg_t msgWrapper;
	msgWrapper.type        = CS_MESH_MODEL_TYPE_ASSET_RSSI_MAC;
	msgWrapper.payload     = reinterpret_cast<uint8_t*>(&asset_msg);
	msgWrapper.size        = sizeof(asset_msg);
	msgWrapper.reliability = CS_MESH_RELIABILITY_LOW;
	msgWrapper.urgency     = CS_MESH_URGENCY_LOW;

	event_t meshMsgEvt(CS_TYPE::CMD_SEND_MESH_MSG, &msgWrapper, sizeof(msgWrapper));

	meshMsgEvt.dispatch();
}

void AssetForwarder::handleEvent(event_t & event) {
	switch (event.type) {
		case CS_TYPE::EVT_RECV_MESH_MSG: {
			auto meshMsg = CS_TYPE_CAST(EVT_RECV_MESH_MSG, event.data);
			if (meshMsg->type == CS_MESH_MODEL_TYPE_ASSET_RSSI_MAC) {
				forwardAssetToUart(meshMsg->getPacket<CS_MESH_MODEL_TYPE_ASSET_RSSI_MAC>());
				event.result.returnCode = ERR_SUCCESS;
			}
			break;
		}
		default:
			break;
	}
}

void AssetForwarder::forwardAssetToUart(const cs_mesh_model_msg_asset_rssi_mac_t& assetMsg) {
	LOGAssetForwarderDebug("forwardAssetToUart");
	printAsset(assetMsg);
	// TODO implement
}

cs_asset_rssi_data_t AssetForwarder::constructUartMsg(
		const cs_mesh_model_msg_asset_rssi_mac_t& assetMsg, const stone_id_t& sender) {
	auto assetData = cs_asset_rssi_data_t{
			.address = {},
			.stoneId = sender,
			.rssi    = getRssiUnsigned(assetMsg.rssiData),
			.channel = getChannel(assetMsg.rssiData),
	};
	memcpy (assetData.address, assetMsg.mac, sizeof(assetMsg.mac));

	return assetData;
}
