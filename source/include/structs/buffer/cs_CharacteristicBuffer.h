/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 2, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <structs/cs_PacketsInternal.h>

/**
 * Size of the header used for long write.
 * Multiple of 4 so the buffer with offset is aligned.
 */
#define CS_CHAR_BUFFER_DEFAULT_OFFSET 8

/**
 * CharacteristicBuffer is a byte array with header.
 *
 * The CharacteristicBuffer is used to put in all kind of data. This data is unorganized. The CharacteristicBuffer can
 * also be accessed through more dedicated structures. This allows to read/write from the buffer directly or other types
 * of sophisticated objects.
 *
 * The disadvantage is that the data will be overwritten by the different accessors. The advantage is that the data
 * fits actually in the device RAM.
 */
class CharacteristicBuffer {

protected:
	buffer_ptr_t _buffer   = nullptr;
	cs_buffer_size_t _size = 0;
	bool _locked           = false;

	CharacteristicBuffer();
	~CharacteristicBuffer();

	//! Copy constructor, singleton, thus made private
	CharacteristicBuffer(CharacteristicBuffer const&)            = delete;

	//! Assignment operator, singleton, thus made private
	CharacteristicBuffer& operator=(CharacteristicBuffer const&) = delete;

public:
	/**
	 * Allocate the buffer.
	 */
	void alloc(cs_buffer_size_t size);

	/**
	 * Clear the buffer.
	 */
	void clear();

	/**
	 * Lock the buffer.
	 *
	 * @return                    True on success.
	 */
	bool lock();

	/**
	 * Unlock the buffer.
	 *
	 * @return                    True on success.
	 */
	bool unlock();

	/**
	 * Check if buffer is locked.
	 *
	 * @return                    True when locked.
	 */
	bool isLocked();

	/**
	 * Get the buffer.
	 *
	 * @param[in] offset          Returned buffer will have this offset from the internal buffer.
	 * @return                    Struct with pointer to the buffer and size of the buffer.
	 */
	cs_data_t getBuffer(cs_buffer_size_t offset = CS_CHAR_BUFFER_DEFAULT_OFFSET);

	/**
	 * Get the buffer.
	 *
	 * @param[out] buffer         Will be set to point to the buffer.
	 * @param[out] size           Will be set to the size of the buffer pointed to.
	 * @param[in] offset          Returned buffer will have this offset from the internal buffer.
	 */
	void getBuffer(buffer_ptr_t& buffer, uint16_t& size, cs_buffer_size_t offset = CS_CHAR_BUFFER_DEFAULT_OFFSET);

	/**
	 * Get size of the buffer.
	 *
	 * @param[in] offset          Offset from the internal buffer, same as in getBuffer().
	 * @return                    Size of the buffer.
	 */
	cs_buffer_size_t size(cs_buffer_size_t offset = CS_CHAR_BUFFER_DEFAULT_OFFSET);
};
