/*
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan. 30, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include <structs/cs_BufferAccessor.h>

#include <protocol/cs_MeshMessageTypes.h>

using MeshCommand = StreamBuffer<uint8_t, MAX_MESH_MESSAGE_LENGTH>;

/** The mesh characteristic message is a wrapped StreamBuffer object.
 *  it uses the type of the StreamBuffer as handle and uses a smaller payload size
 */
//class MeshCommand : public StreamBuffer<uint8_t, MAX_MESH_MESSAGE_PAYLOAD_LENGTH> {

//public:
//	inline uint8_t handle() const { return StreamBuffer::type(); }
//};
