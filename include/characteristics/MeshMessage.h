/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan. 30, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include "ble_gap.h"
#include "ble_gatts.h"

#include "Serializable.h"
#include "BluetoothLE.h"

#define MESH_SERIALIZED_SIZE                     3

class MeshMessage {
private:
	uint8_t _id;
	uint8_t _handle;
	uint8_t _value; // currently just one value in msg
public:
	MeshMessage();

	bool operator!=(const MeshMessage &other);

	uint32_t getSerializedLength() const;

	void serialize(uint8_t *buffer, uint16_t length) const;

	void deserialize(uint8_t *buffer, uint16_t length);

	inline uint8_t id() const { return _id; } 
	inline uint8_t handle() const { return _handle; }
	inline uint8_t value() const { return _value; }
};

// template has to be in the same namespace as the other CharacteristicT templates
namespace BLEpp {

template<> class CharacteristicT<MeshMessage> : public Characteristic<MeshMessage> {

private:
	uint8_t _buffer[MESH_SERIALIZED_SIZE];
	bool _notificationPending;

public:
	CharacteristicT& operator=(const MeshMessage& val) {
		Characteristic<MeshMessage>::operator=(val);
		return *this;
	}

	CharacteristicValue getCharacteristicValue() {
		CharacteristicValue value;
		const MeshMessage& t = this->getValue();
		uint32_t len = t.getSerializedLength();
		memset(_buffer, 0, len);
		t.serialize(_buffer, len);
		return CharacteristicValue(len, _buffer);
	}

	void setCharacteristicValue(const CharacteristicValue& value) {
		MeshMessage t;
		t.deserialize(value.data, value.length);
		this->setValue(t);
	}

	uint16_t getValueMaxLength() {
		return MESH_SERIALIZED_SIZE;
	}

	void onNotifyTxError() {
//		LOGw("[%s] no tx buffers, waiting for BLE_EVT_TX_COMPLETE!", _name.c_str());
		_notificationPending = true;
	}

	void onTxComplete(ble_common_evt_t * p_ble_evt) {
		// if we have a notification pending, try to send it
		if (_notificationPending) {
			uint32_t err_code = notify();
			if (err_code != NRF_SUCCESS) {
//				LOGw("[%s] failed to resend notification!, err_code: %d", _name.c_str(), err_code);
			} else {
//				LOGi("[%s] successfully resent notification", _name.c_str());
				_notificationPending = false;
			}
		}
	}

};

} // end of namespace

