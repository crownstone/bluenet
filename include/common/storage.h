/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 24 Nov., 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#ifndef CS_STORAGE_T
#define CS_STORAGE_T

#include <cstdint>

extern "C" {
// the authors of the Nordic pstorage.h file forgot to include extern "C" wrappers
#include <pstorage.h>
}

/**
 * Implement defaults for storing items. Size is in bytes. The values should respect word-size boundaries (multiple of
 * four).
 */
#define PS_CURRENT_LIMIT        0
#define PS_CURRENT_LIMIT_SIZE   4


/**
 * Class to store items persistently.
 */
class Storage {
public:
	static Storage& getInstance() {
		static Storage instance;
		return instance;
	}

	// Initialize the storage class (in bytes)
	void init(int size);

	void clear();

	// Get byte at location "index" 
	void getUint8(int index, uint8_t *item);

	// Store byte
	void setUint8(int index, const uint8_t item);

	// Get 16-bit integer
	void getUint16(int index, uint16_t *item);

	// Set 16-bit integer
	void setUint16(int index, const uint16_t item);

private:
	Storage();

	// Handle to storage in FLASH
	pstorage_handle_t handle;
	
	pstorage_handle_t block_handle;

	int _size;
};

#endif // CS_STORAGE_T

