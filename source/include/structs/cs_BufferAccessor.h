/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Feb 23, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <common/cs_Types.h>

/** Base class for a buffer accessor object
 *
 * Any object that works on a buffer can use this as a base class.
 */
class BufferAccessor {
public:
	/** Default destructor
	 */
	virtual ~BufferAccessor() {};

	/** Assign the buffer used to hold the data.
	 * @buffer                the buffer to be used
	 * @maxLength             size of buffer (maximum number of bytes that can be stored)
	 *
	 * @return 0 on success
	 */
	virtual int assign(buffer_ptr_t buffer, uint16_t maxLength) = 0; // TODO: should return cs_ret_code_t?

	/** Return the maximum possible length of the object
	 *
	 * @return maximum possible length
	 */
	virtual uint16_t getMaxLength() const = 0;

	/** Return the length of the data in the buffer
	 *
	 * @return data length
	 */
	virtual uint16_t getDataLength() const = 0;

	/** Return the pointer to the buffer and the length of data in Bytes
	 * @buffer       pointer to the buffer
	 * @dataLength   length of data in the buffer (in bytes)
	 */
	virtual void getBuffer(buffer_ptr_t& buffer, uint16_t& dataLength) = 0;

};

