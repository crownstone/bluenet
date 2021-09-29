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
#include <localisation/cs_AssetRecord.h>

#include <protocol/mesh/cs_MeshModelPackets.h>

class AssetForwarder : public EventListener, public Component {
public:
	static constexpr uint16_t MIN_THROTTLED_ADVERTISEMENT_PERIOD_MS = 1000;

	cs_ret_code_t init();

	/**
	 * Sends the mesh messages in the outbox and clears it.
	 * Updates the records throttling counters.
	 *
	 * Messages are sent over both Uart and Mesh
	 */
	void flush();

	/**
	 * Prepare a CS_MESH_MODEL_TYPE_ASSET_RSSI_MAC message and put it
	 * on the outbox. If an identical message is already contained in
	 * the outbox, nothing happens.
	 *
	 * Record is required for throttling. If not available, throttling will not be updated.
	 *
	 * Returns true if there was enough space in the outbox to place the message. Else, false.
	 */
	bool sendAssetMacToMesh(asset_record_t* record, const scanned_device_t& asset);

	/**
	 * Prepare a CS_MESH_MODEL_TYPE_ASSET_INFO message and put it
	 * on the outbox. If an identical message is already contained in
	 * the outbox, nothing happens.
	 *
	 * Returns true if there was enough space in the outbox to place the message. Else, false.
	 */
	bool sendAssetIdToMesh(asset_record_t* record, const scanned_device_t& asset, const asset_id_t& assetId, uint8_t filterBitmask);

	/**
	 * sets how many ticks will be added to records upon sending a message.
	 */
	void setThrottleCountdownBumpTicks(uint8_t ticks);

private:
	stone_id_t _myStoneId;

	uint8_t _throttleCountdownBumpTicks = 0;

	struct outbox_msg_t {
		asset_record_t* rec;
		cs_mesh_model_msg_type_t msgType;
		union {
			cs_mesh_model_msg_asset_report_id_t idMsg;
			cs_mesh_model_msg_asset_report_mac_t macMsg;
			uint8_t rawMsg[MAX_MESH_MSG_PAYLOAD_SIZE];
		};

		// util funcs
		outbox_msg_t();

		/**
		 * returns true if msgType is one of:
		 *   CS_MESH_MODEL_TYPE_ASSET_INFO_MAC
		 *   CS_MESH_MODEL_TYPE_ASSET_INFO_ID
		 */
		bool isValid();

		/**
		 * returns true if they have the same msgType and the addresses are the same.
		 */
		bool isSimilar(const outbox_msg_t& other);
	};

	outbox_msg_t outbox[8] = {};

	/**
	 * validates the message, then
	 * update throttle
	 * send over uart
	 * send over mesh
	 *
	 * returns true if message was valid
	 */
	bool dispatchOutboxMessage(outbox_msg_t outMsg);

	void clearOutbox();

	outbox_msg_t* getEmptyOutboxSlot();
	outbox_msg_t* findSimilar(outbox_msg_t outMsg);

	/**
	 * Forward an asset mesh message to UART.
	 *
	 * @param[in] assetMsg             The mesh message to forward.
	 * @param[in] seenByStoneId        The stone that scanned the asset.
	 */
	void forwardAssetToUart(const cs_mesh_model_msg_asset_report_mac_t& assetMsg, stone_id_t seenByStoneId);
	void forwardAssetToUart(const cs_mesh_model_msg_asset_report_id_t& assetMsg, stone_id_t seenByStoneId);

public:

	/**
	 * Forwards relevant incoming mesh messages to UART.
	 */
	virtual void handleEvent(event_t & event);
};
