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
 */
template<>
class Characteristic<buffer_ptr_t> : public CharacteristicGeneric<buffer_ptr_t> {

private:
	// maximum length for this characteristic in bytes
	uint16_t _maxLength;

	// actual length of data stored in the buffer in bytes
	uint16_t _dataLength;

public:

	/* Set the value for this characteristic
	 * @value the pointer to the buffer in memory
	 *
	 * only valid pointers are allowed, NULL is NOT allowed
	 */
	void setValue(const buffer_ptr_t& value) {
		assert(value, "Error: Don't assign NULL pointers! Pointer to buffer has to be valid from the start!");
		_value = value;
	}

	/* Set the maximum length of the buffer
	 * @length maximum length in bytes
	 *
	 * This defines how many bytes were allocated for the buffer
	 * so how many bytes can max be stored in the buffer
	 */
	void setMaxLength(uint16_t length) {
		_maxLength = length;
	}

	/* Set the length of data stored in the buffer
	 * @length length of data in bytes
	 */
	void setDataLength(uint16_t length) {
		_dataLength = length;
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

	/* Return the actual length of the value
	 *
	 * For a buffer, this is the length of data stored in the buffer in bytes
	 *
	 * @return number of bytes
	 */
	uint16_t getValueLength() {
		return _dataLength;
	}

	/* Return the pointer to the memory where the value is stored
	 *
	 * For a buffer, this is the value itself
	 */
	uint8_t* getValuePtr() {
		return getValue();
	}

protected:

};

} /* namespace BLEpp */



