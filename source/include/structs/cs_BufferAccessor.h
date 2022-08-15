/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Feb 23, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include "structs/cs_PacketsInternal.h"

/**
 * Base class for a buffer accessor object.
 *
 * Any object that works on a buffer can use this as a base class.
 */
class BufferAccessor {
public:
	/**
	 * Default destructor
	 */
	virtual ~BufferAccessor(){};

	/**
	 * Assign the buffer used to hold the data.
	 *
	 * @param[in] buffer          The buffer to be used.
	 * @param[in] size            Size of buffer in bytes.
	 * @return                    ERR_SUCCESS on success.
	 */
	virtual cs_ret_code_t assign(buffer_ptr_t buffer, cs_buffer_size_t size) = 0;

	/**
	 * Get the capacity of the buffer.
	 *
	 * @return                    Maximum size of the buffer in bytes.
	 */
	virtual cs_buffer_size_t getBufferSize() const                           = 0;

	/**
	 * Get the size of the serialized data in the buffer.
	 *
	 * @return                    Size of the serialized data in bytes.
	 */
	virtual cs_buffer_size_t getSerializedSize() const                       = 0;

	virtual cs_buffer_size_t getRemainingCapacity() const { return getBufferSize() - getSerializedSize(); }

	/**
	 * Get the pointer to the buffer.
	 *
	 * @return                    Struct with pointer to buffer and size of the serialized data in bytes.
	 */
	virtual cs_data_t getSerializedBuffer() = 0;
};
