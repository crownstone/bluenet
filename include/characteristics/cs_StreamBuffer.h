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
#include "characteristics/cs_Serializable.h"
#include <string>

#define MAX_BUFFER_SIZE                     20
#define BUFFER_HEADER_SIZE                  2

using namespace BLEpp;

/**
 * General class that can be used to send arrays of values of Bluetooth, of which the first byte is a type
 * or identifier byte, the second one the length of the payload (so, minus identifier and length byte itself)
 * and the next bytes are the payload itself.
 */
class StreamBuffer : public Serializable {
private:
	uint8_t _type;
	uint8_t _plength; // length of payload
	uint8_t* _payload;
public:
	StreamBuffer();

	bool operator!=(const StreamBuffer &other);

	/* Create a string from payload. 
	 *
	 * This creates a C++ string from the uint8_t payload array. Note that this not always makes sense! It will
	 * use the _plength field to establish the length of the array to be used. So, the array does not have to 
	 * have a null-terminated byte.
	 */
	bool toString(std::string &str);

	/* Fill payload with string.
	 */
	bool fromString(const std::string & str);

	// return 0 on SUCCESS, positive value on FAILURE
	uint8_t add(uint8_t value);

	void clear();

	inline uint8_t type() const { return _type; } 

	/* Return the length/size of the payload.
	 */
	inline uint8_t length() const { return _plength; }
	inline uint8_t* payload() const { return _payload; }

	inline void setType(uint8_t type) { _type = type; }

	/* Set payload of the buffer.
	 *
	 * Note that plength here is payload length.
	 */
	void setPayload(uint8_t *payload, uint8_t plength);

	//////////// serializable ////////////////////////////

	/* @inherit */
    uint16_t getSerializedLength() const;

	/* @inherit */
    uint16_t getMaxLength() const {
		return MAX_BUFFER_SIZE + BUFFER_HEADER_SIZE;
    }

	/* Serialize entire class.
	 *
	 * This is used to stream this class over BLE. Note that length here includes the field for type and length,
	 * and is hence larger than plength (which is just the length of the payload).
	 */
    void serialize(uint8_t* buffer, uint16_t length) const;

	/* @inherit */
    void deserialize(uint8_t* buffer, uint16_t length);

};

//// template has to be in the same namespace as the other CharacteristicT templates
//namespace BLEpp {
//
//template<> class CharacteristicT<StreamBuffer> : public Characteristic<StreamBuffer> {
//
//private:
//	uint8_t _buffer[MAX_BUFFER_SIZE];
//	bool _notificationPending;
//
//public:
//	CharacteristicT& operator=(const StreamBuffer& val) {
//		Characteristic<StreamBuffer>::operator=(val);
//		return *this;
//	}
//
//	CharacteristicValue getCharacteristicValue() {
//		CharacteristicValue value;
//		const StreamBuffer& t = this->getValue();
//		uint32_t len = t.getSerializedLength();
//		memset(_buffer, 0, len);
//		t.serialize(_buffer, len);
//		return CharacteristicValue(len, _buffer);
//	}
//
//	void setCharacteristicValue(const CharacteristicValue& value) {
//		StreamBuffer t;
//		t.deserialize(value.data, value.length);
//		this->setValue(t);
//	}
//
//	uint16_t getValueMaxLength() {
//		return MAX_BUFFER_SIZE + BUFFER_HEADER_SIZE;
//	}
//
//	void onNotifyTxError() {
//		_notificationPending = true;
//	}
//
//	void onTxComplete(ble_common_evt_t * p_ble_evt) {
//		if (_notificationPending) {
//			uint32_t err_code = notify();
//			if (err_code == NRF_SUCCESS) {
//				_notificationPending = false;
//			}
//		}
//	}
//
//};
//
//} // end of namespace

