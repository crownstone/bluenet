/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Feb 23, 2015
 * License: LGPLv3+
 */
#pragma once

#include "util/cs_Utils.h"

namespace BLEpp {

/* Base class for a serializable object
 *
 * Any characteristic that needs to send a non primitive type
 * can use this as a base class. The class has to implement
 * serialize and deserialize functions where the variables of
 * the class are written to / read from a byte buffer.
 *
 */
class BufferAccessor {
public:
	/* Default destructor
	 */
	virtual ~BufferAccessor() {};

	/* Return the maximum possible length of the object
	 *
	 * @return maximum possible length
	 */
	virtual uint16_t getMaxLength() const = 0;

	/* Return the length of the data in the buffer
	 *
	 * @return data length
	 */
	virtual uint16_t getDataLength() const = 0;

	/* Return the pointer to the buffer and the length of data in Bytes
	 */
	virtual void getBuffer(uint8_t** buffer, uint16_t& dataLength) = 0;

};

/* This template implements the functions specific for a Characteristic with
 * and handles notify requests for the characteristic, in particular making sure
 * that the object is sent over the air.
 */
template<>
class CharacteristicT<BufferAccessor*> : public CharacteristicGeneric<BufferAccessor*> {

private:

public:

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
		uint8_t* buffer;
		uint16_t len;
		_value->getBuffer(&buffer, len);
		BLEutil::printArray(buffer, len);
		return CharacteristicValue(len, buffer, false);
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
		uint8_t* buffer;
		uint16_t len;
		_value->getBuffer(&buffer, len);
		if (buffer != NULL) {
			memcpy(buffer, value.data, value.length);
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
		return _value->getMaxLength();
	}

};

} /* namespace BLEpp */
