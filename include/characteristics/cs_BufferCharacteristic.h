/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 3, 2015
 * License: LGPLv3+
 */
#pragma once

#include "util/cs_Utils.h"

namespace BLEpp {

/* This template implements the functions specific for a pointer to a buffer
 * It takes care of setting and getting the buffer
 * and handles notify requests for the characteristic, in particular making sure
 * that the object is sent over the air.
 */
template<>
class CharacteristicT<uint8_t*> : public Characteristic<uint8_t*> {

private:
	// maximum length for this characteristic in bytes
	uint16_t _maxLength;

	// actual length of data stored in the buffer in bytes
	uint16_t _dataLength;

public:

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
		LOGd("getCharacteristicValue");
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
		LOGd("setCharacteristicValue");
		if (_value != NULL) {
			memcpy(_value, value.data, value.length);
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

protected:

};

} /* namespace BLEpp */



