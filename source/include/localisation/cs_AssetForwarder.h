/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 12, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <localisation/cs_AssetHandler.h>
#include <events/cs_EventListener.h>
#include <drivers/cs_Timer.h>
#include <structs/buffer/cs_CircularBuffer.h>

class AssetForwarder : public EventListener, AssetHandlerMac {
public:
	AssetForwarder();

	cs_ret_code_t init();

	virtual void handleAcceptedAsset(AssetFilter f, const scanned_device_t& asset);

	virtual void handleEvent(event_t & event);

private:
	stone_id_t _myId = 0;

	void forwardAssetToUart(const cs_mesh_model_msg_asset_rssi_mac_t& assetMsg, stone_id_t sender);
	cs_asset_rssi_data_t constructUartMsg(const cs_mesh_model_msg_asset_rssi_mac_t& assetMsg,
			 stone_id_t sender);

	void forwardToMesh(cs_mesh_model_msg_asset_rssi_mac_t& msg);


	CircularBuffer<cs_mesh_model_msg_asset_rssi_mac_t> _msgQueue;
	app_timer_t              _timerData;
	app_timer_id_t           _timerId = nullptr;
	void onTimer();

public:
	static void staticOnTimer(AssetForwarder *ptr) {
		ptr->onTimer();
	}
};
