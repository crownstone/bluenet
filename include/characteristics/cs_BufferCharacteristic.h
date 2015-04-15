/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 3, 2015
 * License: LGPLv3+
 */
#pragma once

#include "util/cs_Utils.h"

namespace BLEpp {

/* This template implements the functions specific for a pointer to a buffer.
 *
 * It takes care of setting and getting the buffer
 * and handles notify requests for the characteristic, in particular making sure
 * that the object is sent over the air.
 */
template<>
class Characteristic<uint8_t*> : public CharacteristicGeneric<uint8_t*> {

private:
	typedef uint8_t* buffer_ptr_t;

	// maximum length for this characteristic in bytes
	uint16_t _maxLength;

	// actual length of data stored in the buffer in bytes
	uint16_t _dataLength;

public:

	void setValue(const buffer_ptr_t& value) {
		assert(value, "Error: Don't assign NULL pointers! Pointer to buffer has to be correct from the start!");
		_value = value;
	}

	void setMaxLength(uint16_t length) {
		_maxLength = length;
	}

	void setDataLength(uint16_t length) {
		_dataLength = length;
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
		return CharacteristicValue(_dataLength, _value, false);
	}

	/* Assign the given <CharacteristicValue> to this characteristic
	 *
	 * @value the <CharacteristicValue> object which should be assigned
	 *
	 * Deserializes the byte buffer obtained from the <CharacteristicValue>
	 * into an object and assigns that to the charachteristic
	 */
	void setCharacteristicValue(const CharacteristicValue& value) {
		if (_value != NULL) {
			memcpy(_value, value.data, value.length);
			_dataLength = value.length;
		} else {
			LOGe("_value is NULL");
		}
	}

	/* Return the maximum possible length of the buffer
	 *
	 * Checks the object assigned to this characteristics for the maximum
	 * possible length
	 *
	 * @return the maximum possible length
	 */
	uint16_t getValueMaxLength() {
		return _maxLength;
	}

	uint16_t getValueDataLength() {
		return _dataLength;
	}

protected:

};

} /* namespace BLEpp */



