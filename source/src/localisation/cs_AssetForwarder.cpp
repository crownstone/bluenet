/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 12, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <localisation/cs_AssetForwarder.h>
#include <mesh/cs_MeshMsgEvent.h>

#include <protocol/cs_RssiAndChannel.h>
#include <protocol/cs_Packets.h>
#include <uart/cs_UartHandler.h>
#include <logging/cs_Logger.h>
#include <storage/cs_State.h>

#define LOGAssetForwarderDebug LOGvv

void printAsset(const cs_mesh_model_msg_asset_info_mac_t& assetMsg) {
	LOGAssetForwarderDebug("mesh_model_msg_asset: ch%u @ %i dB",
			getChannel(assetMsg.rssiData),
			getRssi(assetMsg.rssiData));
}


cs_ret_code_t AssetForwarder::init() {
	State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &_myStoneId, sizeof(_myStoneId));
	listen();
	return ERR_SUCCESS;
}

uint16_t AssetForwarder::sendAssetMacToMesh(const scanned_device_t& asset) {
	LOGAssetForwarderDebug("Forward mac-over-mesh ch%u, %d dB", asset.channel, asset.rssi);
	cs_mesh_model_msg_asset_info_mac_t assetMsg;

	assetMsg.rssiData = rssi_and_channel_t(asset.rssi, asset.channel);
	memcpy(assetMsg.mac, asset.address, sizeof(assetMsg.mac));

	cs_mesh_msg_t msgWrapper;
	msgWrapper.type        = CS_MESH_MODEL_TYPE_ASSET_INFO_MAC;
	msgWrapper.payload     = reinterpret_cast<uint8_t*>(&assetMsg);
	msgWrapper.size        = sizeof(assetMsg);
	msgWrapper.reliability = CS_MESH_RELIABILITY_LOW;
	msgWrapper.urgency     = CS_MESH_URGENCY_LOW;

	event_t meshMsgEvt(CS_TYPE::CMD_SEND_MESH_MSG, &msgWrapper, sizeof(msgWrapper));
	meshMsgEvt.dispatch();

	// forward message over uart (i.e. it's also interesting if the hub dongle directly receives asset advertisement)
	forwardAssetToUart(assetMsg, _myStoneId);

	return MIN_THROTTLED_ADVERTISEMENT_PERIOD_MS;
}


uint16_t AssetForwarder::sendAssetIdToMesh(const scanned_device_t& asset, const short_asset_id_t& assetId) {
	LOGAssetForwarderDebug("Forward sid-over-mesh ch%u, %d dB", asset.channel, asset.rssi);
	cs_mesh_model_msg_asset_info_id_t assetMsg = {};

	assetMsg.rssiData = rssi_and_channel_t(asset.rssi, asset.channel);
	assetMsg.id = assetId;

	cs_mesh_msg_t msgWrapper;
	msgWrapper.type        = CS_MESH_MODEL_TYPE_ASSET_INFO_ID;
	msgWrapper.payload     = reinterpret_cast<uint8_t*>(&assetMsg);
	msgWrapper.size        = sizeof(assetMsg);
	msgWrapper.reliability = CS_MESH_RELIABILITY_LOW;
	msgWrapper.urgency     = CS_MESH_URGENCY_LOW;

	event_t meshMsgEvt(CS_TYPE::CMD_SEND_MESH_MSG, &msgWrapper, sizeof(msgWrapper));
	meshMsgEvt.dispatch();

	// forward message over uart (i.e. it's also interesting if the hub dongle directly receives asset advertisement)
	forwardAssetToUart(assetMsg, _myStoneId);

	return MIN_THROTTLED_ADVERTISEMENT_PERIOD_MS;
}


void AssetForwarder::handleEvent(event_t & event) {
	switch (event.type) {
		case CS_TYPE::EVT_RECV_MESH_MSG: {
			auto meshMsg = CS_TYPE_CAST(EVT_RECV_MESH_MSG, event.data);

			switch(meshMsg->type) {
				case CS_MESH_MODEL_TYPE_ASSET_INFO_MAC: {
					forwardAssetToUart(meshMsg->getPacket<CS_MESH_MODEL_TYPE_ASSET_INFO_MAC>(), meshMsg->srcAddress);
					event.result.returnCode = ERR_SUCCESS;
					break;
				}
				case CS_MESH_MODEL_TYPE_ASSET_INFO_ID: {
					forwardAssetToUart(meshMsg->getPacket<CS_MESH_MODEL_TYPE_ASSET_INFO_ID>(), meshMsg->srcAddress);
					event.result.returnCode = ERR_SUCCESS;
					break;
				}
				default: {
					break;
				}
			}
			break;
		}
		default:
			break;
	}
}

void AssetForwarder::forwardAssetToUart(const cs_mesh_model_msg_asset_info_mac_t& assetMsg, stone_id_t seenByStoneId) {
	printAsset(assetMsg);

	auto uartAssetMsg = cs_asset_rssi_data_mac_t {
			.address = {},
			.stoneId = seenByStoneId,
			.rssi    = assetMsg.rssiData.getRssi(),
			.channel = assetMsg.rssiData.getChannel(),
	};
	memcpy(uartAssetMsg.address, assetMsg.mac, sizeof(assetMsg.mac));

	UartHandler::getInstance().writeMsg(
			UartOpcodeTx::UART_OPCODE_TX_ASSET_RSSI_MAC_DATA,
			reinterpret_cast<uint8_t*>(&uartAssetMsg),
			sizeof(uartAssetMsg));
}

void AssetForwarder::forwardAssetToUart(const cs_mesh_model_msg_asset_info_id_t& assetMsg, stone_id_t seenByStoneId) {
	LOGAssetForwarderDebug("forwarding sid asset msg to uart");

	auto uartAssetMsg = cs_asset_rssi_data_id_t{
			.assetId = assetMsg.id,
			.stoneId = seenByStoneId,
			.rssi    = assetMsg.rssiData.getRssi(),
			.channel = assetMsg.rssiData.getChannel(),
	};

	UartHandler::getInstance().writeMsg(
			UartOpcodeTx::UART_OPCODE_TX_ASSET_RSSI_SID_DATA,
			reinterpret_cast<uint8_t*>(&uartAssetMsg),
			sizeof(uartAssetMsg));
}
