/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 24 Nov., 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#ifndef CS_STORAGE_T
#define CS_STORAGE_T

#include <cstdint>
#include <cstdio>
#include <string>

extern "C" {
// the authors of the Nordic pstorage.h file forgot to include extern "C" wrappers
#include <pstorage.h>
}

#include <pstorage_platform.h>
#include <drivers/serial.h>
#include <util/utils.h>

/**
 * To add a new element to the storage requires the following steps:
 *
 *   1. define an ID for the service in the enum ps_storage_id
 *   2. define a new struct with the elements that should be stored for
 *      this service, with baseclass ps_storage_base_t
 *   3. add a new element to the config array found in storage.cpp
 *      in the form of:
 *   	  {ENUM_ID, {}, sizeof(struct)}
 */

// define the maximum size for strings to be stored
#define MAX_STRING_SIZE 32

struct ps_storage_base_t {

	// helper function to convert std::string to char array
	void setString(std::string value, char* target) {
		if (value.length() < MAX_STRING_SIZE) {
			memset(target, 0, MAX_STRING_SIZE);
			memcpy(target, value.c_str(), value.length());
		}
	}

	// helper function to get std::string from char array, or default value
	// if the value read is empty, unassigned (filled with FF) or too long
	void getString(char* value, std::string& target, std::string default_value) {
		target = std::string(value);
		// use MAX_STRING_SIZE - 1 to avoid reading in area from the
		// flash which has not been written yet and is all FF
		if (target == "" || target.length() > MAX_STRING_SIZE - 1) {
			LOGd("use default value");
			target = default_value;
		}
		LOGd("found stored value: %s", target.c_str());
	}
};

enum ps_storage_id {
	PS_ID_POWER_SERVICE = 0,
	PS_ID_GENERAL_SERVICE
};

struct storage_config_t {
	ps_storage_id id;
	pstorage_handle_t handle;
	uint8_t storage_size;
};

// POWER SERVICE //////////////////
struct ps_power_service_t : ps_storage_base_t {
	uint32_t current_limit;
};

// GENERAL SERVICE ///////////////
struct ps_general_service_t : ps_storage_base_t {
	char device_name[MAX_STRING_SIZE];
	char room[MAX_STRING_SIZE];
	char device_type[MAX_STRING_SIZE];
};

/////////////////

/**
 * Class to store items persistently.
 */
class Storage {

public:
	static Storage& getInstance() {
		static Storage instance;
		return instance;
	}

	// get the storage handle for the given ID. has to be defined
	//   above
	bool getHandle(ps_storage_id storageID, pstorage_handle_t& handle);

	void clearBlock(pstorage_handle_t handle, int blockID);

	// Get byte at location "index" 
	void getUint8(pstorage_handle_t handle, int blockID, uint8_t *item);

	// Store byte
	void setUint8(pstorage_handle_t handle, int blockID, const uint8_t item);

	// Get 16-bit integer
	void getUint16(pstorage_handle_t handle, int blockID, uint16_t *item);

	// Set 16-bit integer
	void setUint16(pstorage_handle_t handle, int blockID, const uint16_t item);

	// Set/Get string
	void getString(pstorage_handle_t handle, int blockID, char* item, int16_t length);
	void setString(pstorage_handle_t handle, int blockID, const char* item, int16_t length);

	// Set/Get struct
	void getStruct(pstorage_handle_t handle, ps_storage_base_t* item, uint16_t size);
	void setStruct(pstorage_handle_t handle, ps_storage_base_t* item, uint16_t length);
private:
	Storage();

	// Initialize the storage class (in bytes)
	void initBlocks(int blocks, int size, pstorage_handle_t& handle);

	int _size;
};

#endif // CS_STORAGE_T

