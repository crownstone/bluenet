/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan. 30, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include "ble_gap.h"
#include "ble_gatts.h"

#include "cs_BluetoothLE.h"
#include "cs_Serializable.h"

#define MAX_BUFFER_SERIALIZED_SIZE                     20

/**
 * General class that can be used to send arrays of values of Bluetooth, of which the first byte is a type
 * or identifier byte, the second one the length of the payload (so, minus identifier and length byte itself)
 * and the next bytes are the payload itself.
 */
class StreamBuffer {
private:
	uint8_t _type;
	uint8_t _length; // length of payload
	uint8_t* _payload;
public:
	StreamBuffer();

	bool operator!=(const StreamBuffer &other);

	uint32_t getSerializedLength() const;

	// return 0 on SUCCESS, positive value on FAILURE
	uint8_t add(uint8_t value);

	void serialize(uint8_t *buffer, uint16_t length) const;

	void deserialize(uint8_t *buffer, uint16_t length);

	inline uint8_t type() const { return _type; } 
	inline uint8_t length() const { return _length; }
	inline uint8_t* payload() const { return _payload; }
};

// template has to be in the same namespace as the other CharacteristicT templates
namespace BLEpp {

template<> class CharacteristicT<StreamBuffer> : public Characteristic<StreamBuffer> {

private:
	uint8_t _buffer[MAX_BUFFER_SERIALIZED_SIZE];
	bool _notificationPending;

public:
	CharacteristicT& operator=(const StreamBuffer& val) {
		Characteristic<StreamBuffer>::operator=(val);
		return *this;
	}

	CharacteristicValue getCharacteristicValue() {
		CharacteristicValue value;
		const StreamBuffer& t = this->getValue();
		uint32_t len = t.getSerializedLength();
		memset(_buffer, 0, len);
		t.serialize(_buffer, len);
		return CharacteristicValue(len, _buffer);
	}

	void setCharacteristicValue(const CharacteristicValue& value) {
		StreamBuffer t;
		t.deserialize(value.data, value.length);
		this->setValue(t);
	}

	uint16_t getValueMaxLength() {
		return MAX_BUFFER_SERIALIZED_SIZE;
	}

	void onNotifyTxError() {
		_notificationPending = true;
	}

	void onTxComplete(ble_common_evt_t * p_ble_evt) {
		if (_notificationPending) {
			uint32_t err_code = notify();
			if (err_code == NRF_SUCCESS) {
				_notificationPending = false;
			}
		}
	}

};

} // end of namespace

