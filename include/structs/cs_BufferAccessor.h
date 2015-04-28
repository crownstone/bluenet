/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Feb 23, 2015
 * License: LGPLv3+
 */
#pragma once

#include <ble/cs_Characteristic.h>
//#include <ble/cs_BluetoothLE.h>
#include <common/cs_Types.h>
//#include "util/cs_Utils.h"

/* Base class for a buffer accessor object
 *
 * Any object that works on a buffer can use this as a base class.
 */
class BufferAccessor {
public:
	/* Default destructor
	 */
	virtual ~BufferAccessor() {};

	/* Assign the buffer used to hold the scanned device list
	 * @buffer                the buffer to be used
	 * @maxLength             size of buffer (maximum number of bytes that can be stored)
	 *
	 * @return 0 on success
	 */
	virtual int assign(buffer_ptr_t buffer, uint16_t maxLength) = 0; // TODO: should return ERR_CODE?

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
	 * @buffer       pointer to the buffer
	 * @dataLength   length of data in the buffer (in bytes)
	 */
	virtual void getBuffer(buffer_ptr_t& buffer, uint16_t& dataLength) = 0;

};

//namespace BLEpp {
//
///* This template implements the functions specific for a Characteristic with
// * a BufferAccessor as the value
// */
//template<>
//class Characteristic<BufferAccessor&> : public CharacteristicGeneric<BufferAccessor&> {
//
//private:
//
//public:
//
//	/* Return the maximum possible length of the buffer
//	 *
//	 * Checks the object assigned to this characteristic for the maximum
//	 * possible length
//	 *
//	 * @return the maximum possible length
//	 */
//	virtual uint16_t getValueMaxLength() {
//		return _value.getMaxLength();
//	}
//
//	/* Return the length of data in the buffer
//	 *
//	 * Checks the object assigned to this characteristic for the
//	 * length of data
//	 *
//	 * @return length of data contained in buffer
//	 */
//	virtual uint16_t getValueLength() {
//		return _value.getDataLength();
//	}
//
//	/* Return the pointer of the buffer
//	 *
//	 * Checks the buffer accessor assigned to this characteristic and
//	 * returns the pointer to the buffer
//	 *
//	 * @return pointer to the buffer
//	 */
//	virtual uint8_t* getValuePtr() {
//		buffer_ptr_t buffer;
//		uint16_t len;
//		_value.getBuffer(buffer, len);
//		return (uint8_t*)buffer;
//	}
//
//};
//
//} /* namespace BLEpp */
