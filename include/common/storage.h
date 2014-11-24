/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 24 Nov., 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#ifndef CS_STORAGE_T
#define CS_STORAGE_T

#include <cstdint>
#include <pstorage.h>

/**
 * Implement defaults for storing items. Size is in bytes.
 */
#define CURRENT_LIMIT        0
#define CURRENT_LIMIT_SIZE   4


/**
 * Class to store items persistently.
 */
class Storage {
public:
	Storage();

	virtual ~Storage();

	// Initialize the storage class (in bytes)
	bool init(int size);

	// Get byte at location "index" 
	inline void getUint8(int index, uint8_t *item) {
		pstorage_load(item, handle, 1, index); 
	}

	// Store byte
	inline void setUint8(int index, uint8_t *item) {
		pstorage_store(handle, item, 1, index);
	}

	// Get 16-bit integer
	inline void getUint16(int index, uint16_t *item) {
		pstorage_load((uint8_t*)item, handle, 2, index); //todo: check endianness
	}

	// Set 16-bit integer
	inline void setUint16(int index, uint16_t *item) {
		pstorage_store(handle, (uint8_t*)item, 2, index);
	}	

private:
	// Handle to storage in FLASH
	pstorage_handle_t *handle;

};

#endif // CS_STORAGE_T

