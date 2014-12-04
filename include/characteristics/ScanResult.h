/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Nov 4, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#ifndef SCANRESULT_H_
#define SCANRESULT_H_

#include "ble_gap.h"
#include "ble_gatts.h"

#include "Serializable.h"
#include "BluetoothLE.h"

struct __attribute__((__packed__)) peripheral_device_t {
	uint8_t addr[BLE_GAP_ADDR_LEN];
	uint16_t occurences;
	int8_t rssi;
};

#define HEADER_SIZE 1 // 1 BYTE for the header = number of elements in the list
#define SERIALIZED_DEVICE_SIZE sizeof(peripheral_device_t) // only works if struct packed

// FIXME BEWARE, because we are using fixed arrays, increasing the size will cause
//   memory and runtime problems.
#define MAX_NR_DEVICES 10

class ScanResult {

private:
//	peripheral_device_t _list[MAX_NR_DEVICES];
	peripheral_device_t* _list;
	uint8_t _freeIdx;

public:
	ScanResult();

	void init();

	bool operator!=(const ScanResult& val);

	void print() const;

	void update(uint8_t * adrs_ptr, int8_t rssi);

	uint16_t getSize() const;

	void reset();

	//////////// serializable ////////////////////////////

    /** Return length of buffer required to store the serialized form of this object.  If this method returns 0,
    * it means that the object does not need external buffer space. */
    uint32_t getSerializedLength() const;

    /** Copy data representing this object into the given buffer.  Buffer will be preallocated with at least
    * getLength() bytes. */
    void serialize(uint8_t* buffer, uint16_t length) const;

    /** Copy data from the given buffer into this object. */
    void deserialize(uint8_t* buffer, uint16_t length);

};

// template has to be in the same namespace as the other CharacteristicT templates
namespace BLEpp {

template<> class CharacteristicT<ScanResult> : public Characteristic<ScanResult> {

private:
	uint8_t _buffer[HEADER_SIZE + MAX_NR_DEVICES * SERIALIZED_DEVICE_SIZE];
	bool _notificationPending;

public:
	CharacteristicT& operator=(const ScanResult& val) {
		Characteristic<ScanResult>::operator=(val);
		return *this;
	}

	CharacteristicValue getCharacteristicValue() {
		CharacteristicValue value;
		const ScanResult& t = this->getValue();
		uint32_t len = t.getSerializedLength();
		memset(_buffer, 0, len);
		t.serialize(_buffer, len);
		return CharacteristicValue(len, _buffer);
	}

	void setCharacteristicValue(const CharacteristicValue& value) {
		ScanResult t;
		t.deserialize(value.data, value.length);
		this->setValue(t);
	}

	uint16_t getValueMaxLength() {
		return BLE_GATTS_VAR_ATTR_LEN_MAX; // TODO
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

}

#endif /* SCANRESULT_H_ */
