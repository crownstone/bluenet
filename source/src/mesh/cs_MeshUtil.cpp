/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 11, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <logging/cs_Logger.h>
#include <mesh/cs_MeshCommon.h>
#include <mesh/cs_MeshUtil.h>
#include <protocol/mesh/cs_MeshModelPacketHelper.h>

namespace MeshUtil {

int8_t getRssi(const nrf_mesh_rx_metadata_t* metaData) {
	switch (metaData->source) {
		case NRF_MESH_RX_SOURCE_SCANNER: return metaData->params.scanner.rssi;
		case NRF_MESH_RX_SOURCE_GATT:
			// TODO: return connection rssi?
			return -10;
			//		case NRF_MESH_RX_SOURCE_FRIEND:
			//			// TODO: is this correct?
			//			return metaData->params.scanner.rssi;
			//			break;
			//		case NRF_MESH_RX_SOURCE_LOW_POWER:
			//			// TODO: is this correct?
			//			return metaData->params.scanner.rssi;
			//			break;
		case NRF_MESH_RX_SOURCE_INSTABURST: return metaData->params.instaburst.rssi; break;
		case NRF_MESH_RX_SOURCE_LOOPBACK: return -1; break;
	}
	return 0;
}

uint8_t getChannel(const nrf_mesh_rx_metadata_t* metaData) {
	switch (metaData->source) {
		case NRF_MESH_RX_SOURCE_SCANNER: return metaData->params.scanner.channel;
		case NRF_MESH_RX_SOURCE_INSTABURST: return metaData->params.instaburst.channel;
		case NRF_MESH_RX_SOURCE_GATT:
			// a semi decent solution: connection index is uint16_t.
			// return metaData->params.gatt.connection_index & 0xff;
			return 0xff;
		case NRF_MESH_RX_SOURCE_LOOPBACK: return 0xff;
	}

	return 0xff;
}

bool getMac(const nrf_mesh_rx_metadata_t* metaData, uint8_t* target) {
	switch (metaData->source) {
		case NRF_MESH_RX_SOURCE_SCANNER: {
			memcpy(target, metaData->params.scanner.adv_addr.addr, MAC_ADDRESS_LEN);
			target[0] += 1;  // Mesh MAC is offset by 1.
			return true;
		}
		case NRF_MESH_RX_SOURCE_INSTABURST:
		case NRF_MESH_RX_SOURCE_GATT:
		case NRF_MESH_RX_SOURCE_LOOPBACK:
		default: return false;
	}
}

MeshMsgEvent fromAccessMessageRX(const access_message_rx_t& accessMsg) {
	MeshMsgEvent msg;
	if (accessMsg.length < MESH_HEADER_SIZE) {
		LOGw("Invalid mesh message of size %u", accessMsg.length);
		msg.type = CS_MESH_MODEL_TYPE_UNKNOWN;
		return msg;
	}

	msg.opCode = static_cast<cs_mesh_model_opcode_t>(accessMsg.opcode.opcode);
	msg.type   = MeshUtil::getType(accessMsg.p_data);
	MeshUtil::getPayload((uint8_t*)accessMsg.p_data, accessMsg.length, msg.msg.data, msg.msg.len);

	msg.srcStoneId      = accessMsg.meta_data.src.value;
	msg.macAddressValid = getMac(accessMsg.meta_data.p_core_metadata, msg.macAddress);
	msg.rssi            = getRssi(accessMsg.meta_data.p_core_metadata);
	msg.channel         = getChannel(accessMsg.meta_data.p_core_metadata);

	// When receiving a TTL:
	// 0 = has not been relayed and will not be relayed
	// 1 = may have been relayed, but will not be relayed
	// 2 - 126 = may have been relayed and can be relayed
	// 127 = has not been relayed and can be relayed
	msg.ttl             = accessMsg.meta_data.ttl;
	//	msg.hops = ACCESS_DEFAULT_TTL - accessMsg.meta_data.ttl;
	msg.isMaybeRelayed  = msg.ttl != 0;

	switch (accessMsg.opcode.opcode) {
		case CS_MESH_MODEL_OPCODE_MSG:
		case CS_MESH_MODEL_OPCODE_UNICAST_RELIABLE_MSG:
		case CS_MESH_MODEL_OPCODE_MULTICAST_RELIABLE_MSG:
		case CS_MESH_MODEL_OPCODE_MULTICAST_NEIGHBOURS: {
			msg.isReply = false;
			break;
		}
		case CS_MESH_MODEL_OPCODE_UNICAST_REPLY:
		case CS_MESH_MODEL_OPCODE_MULTICAST_REPLY: {
			msg.isReply = true;
			break;
		}
	}

	msg.controlCommand = CTRL_CMD_UNKNOWN;
	msg.reply          = nullptr;

	return msg;
}

void printMeshAddress(const char* prefix, const nrf_mesh_address_t* addr) {
	switch (addr->type) {
		case NRF_MESH_ADDRESS_TYPE_INVALID: LOGMeshModelVerbose("%s type=invalid", prefix); break;
		case NRF_MESH_ADDRESS_TYPE_UNICAST: LOGMeshModelVerbose("%s type=unicast id=%u", prefix, addr->value); break;
		case NRF_MESH_ADDRESS_TYPE_VIRTUAL: {
			// 128-bit virtual label UUID,
			__attribute__((unused)) uint32_t* uuid1 = (uint32_t*)(addr->p_virtual_uuid);
			__attribute__((unused)) uint32_t* uuid2 = (uint32_t*)(addr->p_virtual_uuid + 4);
			__attribute__((unused)) uint32_t* uuid3 = (uint32_t*)(addr->p_virtual_uuid + 8);
			__attribute__((unused)) uint32_t* uuid4 = (uint32_t*)(addr->p_virtual_uuid + 12);
			LOGMeshModelVerbose("%s type=virtual id=%u uuid=%x%x%x%x", prefix, addr->value, uuid1, uuid2, uuid3, uuid4);
			break;
		}
		case NRF_MESH_ADDRESS_TYPE_GROUP: LOGMeshModelVerbose("%s type=group id=%u", prefix, addr->value); break;
	}
}

}  // namespace MeshUtil
