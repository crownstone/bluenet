/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Nov 4, 2014
 * License: LGPLv3+
 */

#ifndef SCANRESULT_H_
#define SCANRESULT_H_

#include "ble_gap.h"
#include "ble_gatts.h"

#include "Serializable.h"
#include "BluetoothLE.h"

struct peripheral_device_t {
	uint8_t addr[BLE_GAP_ADDR_LEN];
	char addrs[28];
	uint16_t occurences;
	int8_t rssi;
};

#define HEADER_SIZE 1 // 1 BYTE for the header = number of elements in the list
#define SERIALIZED_DEVICE_SIZE 9
#define MAX_HISTORY 20

class ScanResult {

private:
	peripheral_device_t _list[MAX_HISTORY];
    uint8_t _freeIdx;

public:
	ScanResult();
	virtual ~ScanResult();

	bool operator!=(const ScanResult& val);

	void print() const;

	void update(uint8_t * adrs_ptr, int8_t rssi);

	//////////// serializable ////////////////////////////

    /** Return length of buffer required to store the serialized form of this object.  If this method returns 0,
    * it means that the object does not need external buffer space. */
    virtual uint32_t getSerializedLength() const;

    /** Copy data representing this object into the given buffer.  Buffer will be preallocated with at least
    * getLength() bytes. */
    void serialize(uint8_t* buffer, uint16_t length) const;

    /** Copy data from the given buffer into this object. */
    virtual void deserialize(uint8_t* buffer, uint16_t length);

};

extern ScanResult scanResult;

// template has to be in the same namespace as the other CharacteristicT templates
namespace BLEpp {

template<> class CharacteristicT<ScanResult> : public Characteristic<ScanResult> {

private:
	uint8_t data[MAX_HISTORY * SERIALIZED_DEVICE_SIZE + HEADER_SIZE];

public:

	CharacteristicT& operator=(const ScanResult& val) {
		Characteristic<ScanResult>::operator=(val);
		return *this;
	}

	virtual CharacteristicValue getCharacteristicValue() {
		CharacteristicValue value;
		const ScanResult& t = this->getValue();
		uint32_t len = t.getSerializedLength();
		memset(data, 0, len);
		t.serialize(data, len);
		return CharacteristicValue(len, data, false);
	}

	virtual void setCharacteristicValue(const CharacteristicValue& value) {
		ScanResult t;
		t.deserialize(value.data, value.length);
		this->setValue(t);
	}

	uint16_t getValueMaxLength() {
		return BLE_GATTS_VAR_ATTR_LEN_MAX; // TODO
	}
};

}

#endif /* SCANRESULT_H_ */
