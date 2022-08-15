/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 10, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <common/cs_Types.h>
#include <events/cs_EventListener.h>
#include <mesh/cs_MeshModelSelector.h>
#include <protocol/mesh/cs_MeshModelPackets.h>

/**
 * Class that:
 * - Sends messages to the mesh.
 */
class MeshMsgSender : public EventListener {
public:
	//	/** Callback function definition. */
	//	typedef function<void(const MeshUtil::cs_mesh_queue_item_t& item)> callback_add_t;
	//
	//	/** Callback function definition. */
	//	typedef function<void(const MeshUtil::cs_mesh_queue_item_meta_data_t& item)> callback_rem_t;
	//
	//	void registerAddCallback(const callback_add_t& closure);
	//	void registerRemCallback(const callback_rem_t& closure);
	void init(MeshModelSelector* selector);

	cs_ret_code_t sendMsg(cs_mesh_msg_t* meshMsg);
	cs_ret_code_t sendTestMsg();
	cs_ret_code_t sendNoop(uint8_t transmissions = 0);
	cs_ret_code_t sendMultiSwitchItem(
			const internal_multi_switch_item_t* item,
			const cmd_source_with_counter_t& source,
			uint8_t transmissions = 0);
	cs_ret_code_t sendSetTime(const cs_mesh_model_msg_time_t* item, uint8_t transmissions = 0);
	cs_ret_code_t sendBehaviourSettings(const behaviour_settings_t* item, uint8_t transmissions = 0);
	cs_ret_code_t sendProfileLocation(const cs_mesh_model_msg_profile_location_t* item, uint8_t transmissions = 0);
	cs_ret_code_t sendTrackedDeviceRegister(const cs_mesh_model_msg_device_register_t* item, uint8_t transmissions = 0);
	cs_ret_code_t sendTrackedDeviceToken(const cs_mesh_model_msg_device_token_t* item, uint8_t transmissions = 0);
	cs_ret_code_t sendTrackedDeviceHeartbeat(
			const cs_mesh_model_msg_device_heartbeat_t* item, uint8_t transmissions = 0);
	cs_ret_code_t sendTrackedDeviceListSize(
			const cs_mesh_model_msg_device_list_size_t* item, uint8_t transmissions = 0);

	/** Internal usage */
	void handleEvent(event_t& event);

private:
	//	callback_add_t _addCallback;
	//	callback_rem_t _remCallback;
	MeshModelSelector* _selector;

#if MESH_MODEL_TEST_MSG != 0
	uint32_t _nextSendCounter = 1;
#endif

	cs_ret_code_t handleSendMeshCommand(
			mesh_control_command_packet_t* command, const cmd_source_with_counter_t& source);

	cs_ret_code_t addToQueue(MeshUtil::cs_mesh_queue_item_t& item);
	cs_ret_code_t remFromQueue(MeshUtil::cs_mesh_queue_item_t& item);
};
