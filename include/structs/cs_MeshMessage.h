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

using namespace BLEpp;

struct __attribute__((__packed__)) mesh_message_t {
	uint8_t id;
	uint8_t handle;
	uint8_t value; // currently just one value in msg
};

#define MM_SERIALIZED_SIZE sizeof(mesh_message_t)

class MeshMessage : public BufferAccessor {
private:
	mesh_message_t* _buffer;

public:
	MeshMessage() : _buffer(NULL) {};

	inline uint8_t id() const { return _buffer->id; }
	inline uint8_t handle() const { return _buffer->handle; }
	inline uint8_t value() const { return _buffer->value; }

	//////////// BufferAccessor ////////////////////////////

	/* @inherit */
	int assign(buffer_ptr_t buffer, uint16_t maxLength) {
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
