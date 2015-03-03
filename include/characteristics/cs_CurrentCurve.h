/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Mar. 02, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include "ble_gap.h"
#include "ble_gatts.h"

#include "cs_BluetoothLE.h"
#include "characteristics/cs_Serializable.h"
#include "drivers/cs_ADC.h"
//#include <string>

#define CURRENT_CURVE_SERIALIZED_SIZE (ADC_BUFFER_SIZE*sizeof(uint16_t))
#define CURRENT_CURVE_HEADER_SIZE sizeof(uint16_t)

using namespace BLEpp;

/**
 * Class to send the current curve. The first two bytes is the length of the payload.
 */
class CurrentCurve: public Serializable {
private:
	uint16_t _plength; // length of payload
	uint16_t* _payload;
public:
	CurrentCurve();
	~CurrentCurve();

	/* Allocates memory for the payload, must be called before using any other function!
	 */
	void init();

	bool operator!=(const CurrentCurve &other);

	// return 0 on SUCCESS, positive value on FAILURE
	uint8_t add(uint16_t value);

	/* Clears the payload and sets size to 0
	 */
	void clear();

	/* Return the length/size of the payload.
	 */
	inline uint16_t length() const { return _plength; }
	inline uint16_t* payload() const { return _payload; }


	/* Set payload of the buffer.
	 *
	 * Note that plength here is payload length.
	 */
	void setPayload(uint16_t *payload, uint16_t plength);

	//////////// serializable ////////////////////////////

	/* @inherit */
    uint16_t getSerializedLength() const;

	/* @inherit */
    uint16_t getMaxLength() const {
		return CURRENT_CURVE_SERIALIZED_SIZE + CURRENT_CURVE_HEADER_SIZE;
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
//	uint8_t _buffer[MAX_BUFFER_SERIALIZED_SIZE];
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
//		return MAX_BUFFER_SERIALIZED_SIZE;
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

