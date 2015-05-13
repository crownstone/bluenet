/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan. 30, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

//#include "ble_gap.h"
//#include "ble_gatts.h"
//
//#include "cs_BluetoothLE.h"
#include "structs/cs_BufferAccessor.h"

#include <protocol/cs_MeshMessageTypes.h>

using namespace BLEpp;

#define MM_HEADER_SIZE 4

//#define MM_MAX_DATA_LENGTH MAX_MESH_MESSAGE_LEN - 3
#define MM_MAX_DATA_LENGTH 90

struct __attribute__((__packed__)) mesh_message_t {
	uint8_t handle; // defines the handle or channel on which the data should be sent in the mesh
	uint8_t type; // defines the type of message, i.e. defines the data structure
	uint16_t length; // length of data
	uint8_t data[MM_MAX_DATA_LENGTH];
};


#define MM_SERIALIZED_SIZE sizeof(mesh_message_t)

class MeshMessage : public BufferAccessor {
private:
	mesh_message_t* _buffer;

public:
	MeshMessage() : _buffer(NULL) {};

	inline uint8_t handle() const { return _buffer->handle; }
	inline uint8_t type() const { return _buffer->type; }

	void data(buffer_ptr_t& p_data, uint16_t& length) const {
		p_data = _buffer->data;
		length = _buffer->length;
	}

	//////////// BufferAccessor ////////////////////////////

	/* @inherit */
	int assign(buffer_ptr_t buffer, uint16_t maxLength) {
		LOGi("sizeof(mesh_message_t): %d, maxLength: %d", sizeof(mesh_message_t), maxLength);
		assert(sizeof(mesh_message_t) <= maxLength, "buffer not large enough to hold mesh message!");
		_buffer = (mesh_message_t*)buffer;
		return 0;
	}

	/* @inherit */
	inline uint16_t getDataLength() const {
		return MM_SERIALIZED_SIZE;
	}

	/* @inherit */
	inline uint16_t getMaxLength() const {
		return MM_SERIALIZED_SIZE;
	}

	/* @inherit */
	void getBuffer(buffer_ptr_t& buffer, uint16_t& dataLength) {
		buffer = (buffer_ptr_t)_buffer;
		dataLength = getDataLength();
	}

};
