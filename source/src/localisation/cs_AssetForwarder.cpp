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
#include <uart/cs_UartHandler.h>
#include <logging/cs_Logger.h>
#include <storage/cs_State.h>
#include <drivers/cs_RNG.h>

#define LOGAssetForwarderDebug LOGnone

void printAsset(const cs_mesh_model_msg_asset_rssi_mac_t& assetMsg) {
	LOGAssetForwarderDebug("mesh_model_msg_asset: ch%u @ -%u dB",
			getChannel(assetMsg.rssiData),
			getRssiUnsigned(assetMsg.rssiData));
}

AssetForwarder::AssetForwarder():
		_msgQueue(4)
{}

cs_ret_code_t AssetForwarder::init() {
	State::getInstance().get(CS_TYPE::CONFIG_CROWNSTONE_ID, &_myId, sizeof(_myId));

	_timerData = { {0} };
	_timerId = &_timerData;
	Timer::getInstance().createSingleShot(_timerId, (app_timer_timeout_handler_t)AssetForwarder::staticOnTimer);

	_msgQueue.init();

	listen();
	return ERR_SUCCESS;
}

void AssetForwarder::handleAcceptedAsset(AssetFilter f, const scanned_device_t& asset) {
	cs_mesh_model_msg_asset_rssi_mac_t asset_msg;
	asset_msg.rssiData = compressRssi(asset.rssi, asset.channel);
	memcpy(asset_msg.mac, asset.address, sizeof(asset_msg.mac));
	LOGAssetForwarderDebug("handledAcceptedAsset ch%u, %d dB", asset.channel, asset.rssi);
	printAsset(asset_msg);

	if (_msgQueue.full()) {
		LOGAssetForwarderDebug("Queue full: drop msg");
	}
	else {
		_msgQueue.push(asset_msg);
		Timer::getInstance().start(_timerId, MS_TO_TICKS((RNG::getInstance().getRandom8() % 10) + 1), this);
	}

	// forward message over uart (i.e. it's also interesting if the hub receives asset advertisement)
	forwardAssetToUart(asset_msg, _myId);
}

void AssetForwarder::handleEvent(event_t & event) {
	switch (event.type) {
		case CS_TYPE::EVT_RECV_MESH_MSG: {
			auto meshMsg = CS_TYPE_CAST(EVT_RECV_MESH_MSG, event.data);
			if (meshMsg->type == CS_MESH_MODEL_TYPE_ASSET_RSSI_MAC) {
				forwardAssetToUart(meshMsg->getPacket<CS_MESH_MODEL_TYPE_ASSET_RSSI_MAC>(),
						meshMsg->srcAddress);
				event.result.returnCode = ERR_SUCCESS;
			}
			break;
		}
		default:
			break;
	}
}

void AssetForwarder::forwardAssetToUart(const cs_mesh_model_msg_asset_rssi_mac_t& assetMsg, stone_id_t sender) {
	printAsset(assetMsg);

	cs_asset_rssi_data_t uartAssetMsg = constructUartMsg(assetMsg, sender);

	UartHandler::getInstance().writeMsg(
			UartOpcodeTx::UART_OPCODE_TX_ASSET_RSSI_DATA,
			reinterpret_cast<uint8_t*>(&uartAssetMsg),
			sizeof(uartAssetMsg));
}

cs_asset_rssi_data_t AssetForwarder::constructUartMsg(
		const cs_mesh_model_msg_asset_rssi_mac_t& assetMsg, stone_id_t sender) {
	auto assetData = cs_asset_rssi_data_t{
			.address = {},
			.stoneId = sender,
			.rssi    = getRssi(assetMsg.rssiData),
			.channel = getChannel(assetMsg.rssiData),
	};
	memcpy (assetData.address, assetMsg.mac, sizeof(assetMsg.mac));

	return assetData;
}

void AssetForwarder::forwardToMesh(cs_mesh_model_msg_asset_rssi_mac_t& msg) {
	cs_mesh_msg_t msgWrapper;
	msgWrapper.type        = CS_MESH_MODEL_TYPE_ASSET_RSSI_MAC;
	msgWrapper.payload     = reinterpret_cast<uint8_t*>(&msg);
	msgWrapper.size        = sizeof(msg);
	msgWrapper.reliability = CS_MESH_RELIABILITY_LOW;
	msgWrapper.urgency     = CS_MESH_URGENCY_LOW;

	event_t meshMsgEvt(CS_TYPE::CMD_SEND_MESH_MSG, &msgWrapper, sizeof(msgWrapper));
	meshMsgEvt.dispatch();
}

void AssetForwarder::onTimer() {
	if (_msgQueue.empty()) {
		return;
	}
	auto msg = _msgQueue.pop();
	forwardToMesh(msg);
}
