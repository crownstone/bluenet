/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Feb 23, 2015
 * License: LGPLv3+
 */
#pragma once

namespace BLEpp {

/* Base class for a serializable object
 *
 * Any characteristic that needs to send a non primitive type
 * can use this as a base class. The class has to implement
 * serialize and deserialize functions where the variables of
 * the class are written to / read from a byte buffer.
 *
 */
class Serializable {
public:
	/* Default destructor
	 */
	virtual ~Serializable() {};

	/* Return length of buffer required to store the serialized form of this object.
	 *
	 * @return number of bytes required
	 */
	virtual uint16_t getSerializedLength() const = 0;

	/* Return the maximum possible length of the object
	 *
	 * @return maximum possible length
	 */
	virtual uint16_t getMaxLength() const = 0;

	/* Serialize the data into a byte array
	 *
	 * @buffer buffer in which the data should be copied. **note** the buffer
	 *   has to be preallocated with at least <getSerializedLength> size
	 * @length length of the buffer in bytes
	 *
	 * Copy data representing this object into the given buffer. The buffer
	 * has to be preallocated with at least <getSerializedLength> bytes.
	 *
	 */
	virtual void serialize(uint8_t* buffer, uint16_t length) const = 0;

	/* De-serialize the data from the byte array into this object
	 *
	 * @buffer buffer containing the data of the object. for the form of the data, see
	 * the function <serialize>
	 * @length length of the buffer in bytes
	 *
	 * Copy the data from the given buffer into this object.
	 */
	virtual void deserialize(uint8_t* buffer, uint16_t length) = 0;

};

/* This template implements the functions specific for a Characteristic with
 * a serializable Object. It takes care of setting and getting the serialized object
 * and handles notify requests for the characteristic, in particular making sure
 * that the object is sent over the air.
 */
template<typename T >
class CharacteristicT<T, typename std::enable_if<std::is_base_of<Serializable, T>::value >::type> : public CharacteristicGeneric<T> {

private:
	/* Pointer to buffer used to store the serialized object
	 */
	uint8_t* _buffer;

	/* Flag to indicate if notification is pending to be sent once currently waiting
	 * tx operations are completed
	 */
	bool _notificationPending;

public:
	/* Default destructor
	 */
	virtual ~CharacteristicT() {
		if (_buffer) {
			free(_buffer);
		}
	}

	/* Default assign operator
	 *
	 * @val object which should be assigned
	 *
	 * Assigns the object to this characteristic
	 */
	CharacteristicT& operator=(const T& val) {
		CharacteristicGeneric<T>::operator=(val);
		return *this;
	}

	/* Returns the object currently assigned to the characteristic
	 *
	 * Serializes the object into a byte buffer and returns it as a
	 * <CharacteristicValue>
	 *
	 * @return the serialized object in a <CharacteristicValue> object
	 */
	CharacteristicValue getCharacteristicValue() {
		CharacteristicValue value;
		const T& t = this->getValue();
		uint32_t len = t.getSerializedLength();
//		if (_buffer) {
//			free(_buffer);
//		}
		_buffer = (uint8_t*)calloc(len, sizeof(uint8_t));
		t.serialize(_buffer, len);
		return CharacteristicValue(len, _buffer, true);
	}

	/* Assign the given <CharacteristicValue> to this characteristic
	 *
	 * @value the <CharacteristicValue> object which should be assigned
	 *
	 * Deserializes the byte buffer obtained from the <CharacteristicValue>
	 * into an object and assigns that to the charachteristic
	 */
	void setCharacteristicValue(const CharacteristicValue& value) {
		T t;
		t.deserialize(value.data, value.length);
		this->setValue(t);
	}

	/* Return the maximum possible length of the buffer
	 *
	 * Checks the object assigned to this characteristics for the maximum
	 * possible length
	 *
	 * @return the maximum possible length
	 */
	uint16_t getValueMaxLength() {
		const T& t = this->getValue();
		return t.getMaxLength();
	}

	/* Callback function if a notify tx error occurs
	 *
	 * This is called when the notify operation fails with a tx error. This
	 * can occur when too many tx operations are taking place at the same time.
	 *
	 * A <BLEpp::CharacteristicBase::notify> is called when the master device
	 * connected to the Crownstone requests automatic notifications whenever
	 * the value changes.
	 */
	void onNotifyTxError() {
		_notificationPending = true;
	}

	/* Callback function once tx operations complete
	 *
	 * @p_ble_evt the event object which triggered the onTxComplete callback
	 *
	 * This is called whenever tx operations complete. If a notification is pending
	 * <BLEpp::CharacteristicBase::notify> is called again and the notification
	 * is cleared if the call eas successful. If not successful, it will be tried
	 * again during the next callback call
	 */
	void onTxComplete(ble_common_evt_t * p_ble_evt) {
		// if we have a notification pending, try to send it
		if (_notificationPending) {
			// this-> is necessary so that the call of notify depends on the template
			// parameter T
			uint32_t err_code = this->notify();
			if (err_code == NRF_SUCCESS) {
				_notificationPending = false;
			}
		}
	}

};

} /* namespace BLEpp */
