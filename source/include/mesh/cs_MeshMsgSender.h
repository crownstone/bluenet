/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 10, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <common/cs_Types.h>
#include <events/cs_EventListener.h>
#include <protocol/mesh/cs_MeshModelPackets.h>

/**
 * Class that:
 * - Sends messages to the mesh.
 */
class MeshMsgSender: EventListener {
public:
	/**
	 * Internal usage
	 */
	void handleEvent(event_t & event);

protected:
	cs_ret_code_t sendMsg(cs_mesh_msg_t *meshMsg);
	cs_ret_code_t sendTestMsg();
	cs_ret_code_t sendMultiSwitchItem(const internal_multi_switch_item_t* item, uint8_t repeats=CS_MESH_RELIABILITY_LOW);
	cs_ret_code_t sendTime(const cs_mesh_model_msg_time_t* item, uint8_t repeats=CS_MESH_RELIABILITY_LOWEST);
	cs_ret_code_t sendBehaviourSettings(const behaviour_settings_t* item, uint8_t repeats=CS_MESH_RELIABILITY_LOWEST);
	cs_ret_code_t sendProfileLocation(const cs_mesh_model_msg_profile_location_t* item, uint8_t repeats=CS_MESH_RELIABILITY_LOWEST);
	cs_ret_code_t sendTrackedDeviceRegister(const cs_mesh_model_msg_device_register_t* item, uint8_t repeats=CS_MESH_RELIABILITY_LOW);
	cs_ret_code_t sendTrackedDeviceToken(const cs_mesh_model_msg_device_token_t* item, uint8_t repeats=CS_MESH_RELIABILITY_LOW);
	cs_ret_code_t sendTrackedDeviceListSize(const cs_mesh_model_msg_device_list_size_t* item, uint8_t repeats=CS_MESH_RELIABILITY_LOW);

private:
	cs_ret_code_t addToQueue(cs_mesh_model_msg_type_t type, stone_id_t targetId, uint16_t id, const uint8_t* payload, uint8_t payloadSize, uint8_t repeats, bool priority);
	cs_ret_code_t remFromQueue(cs_mesh_model_msg_type_t type, uint16_t id);
};
