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
//	char addrs[28];
	uint16_t occurences;
	int8_t rssi;
};

#define SERIALIZED_DEVICE_SIZE 9

class ScanResult : public ISerializable {

private:
	peripheral_device_t* _list;
	size_t _size;
    uint8_t _freeIdx;

public:
	ScanResult();
	virtual ~ScanResult();

	bool operator!=(const ScanResult& val);

	// init list for number of devices
	void init(size_t size);

	// return number of devices
	size_t getSize() const;

	peripheral_device_t* getList() const;

	void print() const;

	void update(uint8_t * adrs_ptr, int8_t rssi);

	//////////// ISerializable ////////////////////////////

    /** Return length of buffer required to store the serialized form of this object.  If this method returns 0,
    * it means that the object does not need external buffer space. */
    virtual uint32_t getSerializedLength() const;

    /** Copy data representing this object into the given buffer.  Buffer will be preallocated with at least
    * getLength() bytes. */
    virtual void serialize(Buffer& buffer) const;

    /** Copy data from the given buffer into this object. */
    virtual void deserialize(Buffer& buffer);

};


// template has to be in the same namespace as the other CharacteristicT templates
namespace BLEpp {

template<> class CharacteristicT<ScanResult> : public Characteristic<ScanResult> {

private:
	Buffer mBuffer;

public:

	CharacteristicT& operator=(const ScanResult& val) {
		Characteristic<ScanResult>::operator=(val);
		return *this;
	}

	virtual CharacteristicValue getCharacteristicValue() {
		CharacteristicValue value;
		const ScanResult& t = this->getValue();
		uint32_t len = t.getSerializedLength();
		mBuffer.allocate(len);
		t.serialize(mBuffer);
		mBuffer.release();
		return CharacteristicValue(mBuffer.length, mBuffer.data, true);
	}
	virtual void setCharacteristicValue(const CharacteristicValue& value) {
		ScanResult t;
		Buffer b(value.length, (uint8_t*) value.data);
		t.deserialize(b);
		this->setValue(t);
	}

	uint16_t getValueMaxLength() {
		return BLE_GATTS_VAR_ATTR_LEN_MAX; // TODO
	}
};

}

#endif /* SCANRESULT_H_ */
