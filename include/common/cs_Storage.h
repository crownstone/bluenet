/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 24 Nov., 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */
#pragma once

#include <cstdint>
#include <cstdio>
#include <string>

extern "C" {
// the authors of the Nordic pstorage.h file forgot to include extern "C" wrappers
#include <pstorage.h>
#include <string.h>
}

#include "pstorage_platform.h"
#include "drivers/cs_Serial.h"

/**
 * To add a new element to the storage requires the following steps:
 *
 *   1. define an ID for the service in the enum ps_storage_id
 *   2. define a new struct with the elements that should be stored for
 *      this service, with baseclass ps_storage_base_t
 *   3. add a new element to the config array found in storage.cpp
 *      in the form of:
 *   	  {ENUM_ID, {}, sizeof(struct)}
 *
 *  NOTE: we can't store less than 4 bytes, so even if you only want
 *    to store a byte, the element in the struct has to be defined as
 *    uint32_t
 */

// define the maximum size for strings to be stored
#define MAX_STRING_SIZE                          32
#define MIN_ARRAY_SIZE                           4

struct ps_storage_base_t {

};

enum ps_storage_id {
	PS_ID_POWER_SERVICE = 0,
	PS_ID_GENERAL_SERVICE = 1
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
	uint32_t floor;
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

	// helper functions //////////////////////////////////////////

	// helper function to convert std::string to char array
	static void setString(std::string value, char* target);

	// helper function to get std::string from char array, or default value
	// if the value read is empty or unassigned (filled with FF)
	static void getString(char* value, std::string& target, std::string default_value);

	// helper function to set a byte
	static void setUint8(uint8_t value, uint32_t& target);

	// helper function to read a byte from the storage, or default value
	// if the value is 0 or unassigned (FF)
	static void getUint8(uint32_t value, uint8_t& target, uint8_t default_value);

	static void setUint16(uint16_t value, uint32_t& target);

	static void getUint16(uint32_t value, uint16_t& target, uint16_t default_value);

	static void setUint32(uint32_t value, uint32_t& target);

	static void getUint32(uint32_t value, uint32_t& target, uint32_t default_value);

private:
	Storage();

	// Initialize the storage class (in bytes)
	void initBlocks(int blocks, int size, pstorage_handle_t& handle);

	int _size;
};
