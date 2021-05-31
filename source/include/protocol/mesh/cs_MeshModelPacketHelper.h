/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 10 May., 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <protocol/mesh/cs_MeshModelPackets.h>
#include <structs/cs_PacketsInternal.h>
#include <protocol/cs_Typedefs.h>
#include <protocol/cs_CommandTypes.h>

namespace MeshUtil {

bool isValidMeshMessage(cs_mesh_msg_t* meshMsg);
bool isValidMeshMessage(uint8_t* meshMsg, size16_t msgSize);
bool isValidMeshPayload(cs_mesh_model_msg_type_t type, uint8_t* payload, size16_t payloadSize);
bool testIsValid(const cs_mesh_model_msg_test_t* packet, size16_t size);
bool ackIsValid(const uint8_t* packet, size16_t size);
bool timeIsValid(const cs_mesh_model_msg_time_t* packet, size16_t size);
bool noopIsValid(const uint8_t* packet, size16_t size);
bool multiSwitchIsValid(const uint8_t* packet, size16_t size);
bool state0IsValid(const cs_mesh_model_msg_state_0_t* packet, size16_t size);
bool state1IsValid(const cs_mesh_model_msg_state_1_t* packet, size16_t size);
bool profileLocationIsValid(const cs_mesh_model_msg_profile_location_t* packet, size16_t size);
bool setBehaviourSettingsIsValid(const behaviour_settings_t* packet, size16_t size);

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

/**
 * Get payload of a mesh message.
 *
 * Assumes message is valid.
 * @param[in]      meshMsg        Mesh message..
 * @param[in]      meshMsgSize    Size of the mesh message.
 */
cs_data_t getPayload(uint8_t* meshMsg, size16_t meshMsgSize);

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
 * @param[in]      meshMsgSize    Size of allocated the mesh message.
 * @retval                        True on success.
 */
bool setMeshMessage(cs_mesh_model_msg_type_t type, const uint8_t* payload, size16_t payloadSize, uint8_t* meshMsg, size16_t meshMsgSize);

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

CommandHandlerTypes getCtrlCmdType(cs_mesh_model_msg_type_t meshType);

bool canShortenStateType(uint16_t type);
bool canShortenStateId(uint16_t id);
bool canShortenPersistenceMode(uint8_t id);
bool canShortenAccessLevel(EncryptionAccessLevel accessLevel);
bool canShortenSource(const cmd_source_with_counter_t& source);
bool canShortenRetCode(cs_ret_code_t retCode);

uint8_t getShortenedRetCode(cs_ret_code_t retCode);
cs_ret_code_t getInflatedRetCode(uint8_t retCode);

uint8_t getShortenedAccessLevel(EncryptionAccessLevel accessLevel);
EncryptionAccessLevel getInflatedAccessLevel(uint8_t accessLevel);

uint8_t getShortenedSource(const cmd_source_with_counter_t& source);
cmd_source_with_counter_t getInflatedSource(uint8_t sourceId);

}
