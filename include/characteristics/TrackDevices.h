/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Dec 24, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#ifndef TRACKDEVICES_H_
#define TRACKDEVICES_H_

#include "ble_gap.h"
#include "ble_gatts.h"

#include "Serializable.h"
#include "BluetoothLE.h"

struct __attribute__((__packed__)) tracked_device_t {
	uint8_t addr[BLE_GAP_ADDR_LEN];
	int8_t rssi_threshold;
	uint16_t counter;
};

#define TRACKDEVICES_HEADER_SIZE 1 // 1 BYTE for the header = number of elements in the list
#define TRACKDEVICES_SERIALIZED_SIZE (sizeof(tracked_device_t)-2) // only works if struct packed. Subtract 2, as counter is not serialized

#define TRACKDEVICES_MAX_NR_DEVICES              2
#define COUNTER_MAX                              5
#define ALONE_THRESHOLD							 2000
	
#define TDL_IS_ALONE                             1
#define TDL_SOMEONE_NEARBY                       2
#define TDL_NOT_TRACKING                         3

class TrackedDeviceList {

private:
	tracked_device_t* _list;
	uint8_t _freeIdx;

public:
	TrackedDeviceList();

	/** Initializes the list, must be called before any other function! */
	void init();

	bool operator!=(const TrackedDeviceList& val);

	/** No entity to be tracked nearby  */
	uint8_t isAlone();

	/** Print a single address */
	void print(uint8_t *addr) const;

	/** Prints the list. */
	void print() const;

	void update(const uint8_t * adrs_ptr, int8_t rssi);

	/** Returns the current size of the list */
	uint16_t getSize() const;

	/** Clears the list. */
	void reset();

	/** Adds/updates an address with given rssi threshold to/in the list. Returns true on success. */
	bool add(const uint8_t* adrs_ptr, int8_t rssi_threshold);

	/** Removes an address from the list. Returns true on success, false when it's not in the list. */
	bool rem(uint8_t* adrs_ptr);

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



class TrackedDevice {

private:
	tracked_device_t _trackedDevice;

public:
	TrackedDevice();

	void init();

	bool operator!=(const TrackedDevice& val);

	void print() const;

	int8_t getRSSI() const { return _trackedDevice.rssi_threshold; }
	
	const uint8_t* getAddress() const { return _trackedDevice.addr; }

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

template<> class CharacteristicT<TrackedDeviceList> : public Characteristic<TrackedDeviceList> {

private:
	uint8_t _buffer[TRACKDEVICES_HEADER_SIZE + TRACKDEVICES_MAX_NR_DEVICES * TRACKDEVICES_SERIALIZED_SIZE];
	bool _notificationPending;

public:
	CharacteristicT& operator=(const TrackedDeviceList& val) {
		Characteristic<TrackedDeviceList>::operator=(val);
		return *this;
	}

	CharacteristicValue getCharacteristicValue() {
		CharacteristicValue value;
		const TrackedDeviceList& t = this->getValue();
		uint32_t len = t.getSerializedLength();
		memset(_buffer, 0, len);
		t.serialize(_buffer, len);
		return CharacteristicValue(len, _buffer);
	}

	void setCharacteristicValue(const CharacteristicValue& value) {
		TrackedDeviceList t;
		t.deserialize(value.data, value.length);
		this->setValue(t);
	}

	uint16_t getValueMaxLength() {
		return TRACKDEVICES_HEADER_SIZE + TRACKDEVICES_MAX_NR_DEVICES * TRACKDEVICES_SERIALIZED_SIZE;
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



template<> class CharacteristicT<TrackedDevice> : public Characteristic<TrackedDevice> {

private:
	uint8_t _buffer[TRACKDEVICES_SERIALIZED_SIZE];
	bool _notificationPending;

public:
	CharacteristicT& operator=(const TrackedDevice& val) {
		Characteristic<TrackedDevice>::operator=(val);
		return *this;
	}

	CharacteristicValue getCharacteristicValue() {
		CharacteristicValue value;
		const TrackedDevice& t = this->getValue();
		uint32_t len = t.getSerializedLength();
		memset(_buffer, 0, len);
		t.serialize(_buffer, len);
		return CharacteristicValue(len, _buffer);
	}

	void setCharacteristicValue(const CharacteristicValue& value) {
		TrackedDevice t;
		t.deserialize(value.data, value.length);
		this->setValue(t);
	}

	uint16_t getValueMaxLength() {
		return TRACKDEVICES_SERIALIZED_SIZE;
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

} // namespace BLEpp

#endif /* TRACKDEVICES_H_ */
