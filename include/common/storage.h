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
 * Implement defaults for storing items. Size is in bytes.
 */
#define PS_CURRENT_LIMIT        0
#define PS_CURRENT_LIMIT_SIZE   2


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
	void getUint8(int index, uint8_t *item);

	// Store byte
	void setUint8(int index, uint8_t *item);

	// Get 16-bit integer
	void getUint16(int index, uint16_t *item);

	// Set 16-bit integer
	void setUint16(int index, const uint16_t *item);

private:
	// Handle to storage in FLASH
//	pstorage_handle_t handle;

};

#endif // CS_STORAGE_T

