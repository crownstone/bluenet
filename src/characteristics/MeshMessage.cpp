/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan. 30, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include <characteristics/MeshMessage.h>


MeshMessage::MeshMessage() {
}

bool MeshMessage::operator!=(const MeshMessage &other) {
	if (_id != other._id) return true;
	if (_channel != other._channel) return true;
	if (_value != other._value) return true;
	return false;
}

uint32_t MeshMessage::getSerializedLength() const {
	return MESH_SERIALIZED_SIZE;
}

void MeshMessage::serialize(uint8_t* buffer, uint16_t length) const {
	if (length < 3) return; // throw error
	
	uint8_t *ptr = buffer;
	*ptr++ = _id;
	*ptr++ = _channel;
	*ptr++ = _value;
}

void MeshMessage::deserialize(uint8_t* buffer, uint16_t length) {
	if (length < 3) return;

	_id = buffer[0];
	_channel = buffer[1];
	_value = buffer[2];
}

