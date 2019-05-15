/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 10 May., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include "protocol/mesh/cs_MeshModelPackets.h"
#include "protocol/cs_Typedefs.h"

namespace MeshModelPacketHelper {

bool isValidMeshMessage(uint8_t* meshMsg, size16_t msgSize);
bool testIsValid(const cs_mesh_model_msg_test_t* packet, size16_t size);
bool ackIsValid(const uint8_t* packet, size16_t size);
bool timeIsValid(const cs_mesh_model_msg_time_t* packet, size16_t size);
bool noopIsValid(const uint8_t* packet, size16_t size);
bool multiSwitchIsValid(const cs_mesh_model_msg_multi_switch_t* packet, size16_t size);
bool keepAliveStateIsValid(const cs_mesh_model_msg_keep_alive_t* packet, size16_t size);
bool keepAliveIsValid(const uint8_t* packet, size16_t size);

cs_mesh_model_msg_type_t getType(const uint8_t* meshMsg);

/**
 * Get payload of a mesh message.
 *
 * Assumes message is valid.
 * @param[in]      meshMsg        Mesh message..
 * @param[in]      meshMsgSize    Size of the mesh message.
 * @param[out]     payload        Set to payload.
 * @param[out]     payloadSize    Set to size of the payload.
 */
void getPayload(uint8_t* meshMsg, size16_t meshMsgSize, uint8_t*& payload, size16_t& payloadSize);

size16_t getMeshMessageSize(size16_t payloadSize);

/**
 * Create a mesh message.
 *
 * Copies the payload into the mesh message.
 * Assumes payload is valid.
 * @param[in]      type           Payload type.
 * @param[in]      payload        Payload packet.
 * @param[in]      payloadSize    Size of the payload.
 * @param[in,out]  meshMsg        Mesh message, must already be allocated.
 * @param[in,out]  meshMsgSize    Size of allocated the mesh message, set to size of the message on success.
 * @retval                        True on success.
 */
bool setMeshMessage(cs_mesh_model_msg_type_t type, const uint8_t* payload, size16_t payloadSize, uint8_t* meshMsg, size16_t& meshMsgSize);

/**
 * Set payload of a mesh message.
 *
 * Copies the payload into the mesh message.
 * Assumes payload is valid.
 * @param[in]      payload        Payload packet.
 * @param[in]      payloadSize    Size of the payload.
 * @param[in,out]  meshMsg        Mesh message, must already be allocated.
 * @param[in,out]  meshMsgSize    Size of allocated the mesh message, set to size of the message on success.
 * @retval                        True on success.
 */
bool setMeshPayload(uint8_t* meshMsg, size16_t meshMsgSize, const uint8_t* payload, size16_t payloadSize);

/**
 * Search for item with given stone id.
 *
 * Assumes packet is valid.
 * @param[in]  msg      The message.
 * @param[in]  stoneId  Stone id to search for.
 * @param[out] item     Set to item with given stoneId.
 * @retval              true when stone id was found.
 */
bool multiSwitchHasItem(cs_mesh_model_msg_multi_switch_t* packet, stone_id_t stoneId, cs_mesh_model_msg_multi_switch_item_t*& item);
bool keepAliveHasItem(cs_mesh_model_msg_keep_alive_t* packet, stone_id_t stoneId, cs_mesh_model_msg_keep_alive_item_t*& item);

}
