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


/**
 * This defines the layout of the mesh characteristic. in addition to the mesh messages, defined in
 * cs_MeshMessageTypes.h, the characteristic also expects the channel = handle, on which the
 * mesh message should be sent, as well as the length of the mesh message.
 * the data part of the mesh characteristic message, is the actual mesh message, which can be sent,
 * as is, into the mesh network on the defined channel
 */

using namespace BLEpp;

#define MM_HEADER_SIZE 4

//#define MM_MAX_DATA_LENGTH MAX_MESH_MESSAGE_LEN - 3
#define MM_MAX_DATA_LENGTH 90

struct __attribute__((__packed__)) mesh_characteristic_message_t {
	uint8_t channel; // defines the handle or channel on which the data should be sent in the mesh
//	uint8_t type; // defines the type of message, i.e. defines the data structure
	uint8_t reserverd;
	uint16_t length; // length of data
	uint8_t data[MM_MAX_DATA_LENGTH];
};


#define MM_SERIALIZED_SIZE sizeof(mesh_characteristic_message_t)

class MeshCharacteristicMessage : public BufferAccessor {
private:
	mesh_characteristic_message_t* _buffer;

public:
	MeshCharacteristicMessage() : _buffer(NULL) {};

	inline uint8_t channel() const { return _buffer->channel; }
//	inline uint8_t type() const { return _buffer->type; }

	void data(buffer_ptr_t& p_data, uint16_t& length) const {
		p_data = _buffer->data;
		length = _buffer->length;
	}

	//////////// BufferAccessor ////////////////////////////

	/* @inherit */
	int assign(buffer_ptr_t buffer, uint16_t maxLength) {
		LOGi("sizeof(mesh_characteristic_message_t): %d, maxLength: %d", sizeof(mesh_characteristic_message_t), maxLength);
		assert(sizeof(mesh_characteristic_message_t) <= maxLength, "buffer not large enough to hold mesh message!");
		_buffer = (mesh_characteristic_message_t*)buffer;
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
