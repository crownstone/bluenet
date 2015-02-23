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
 * Adding a new element to the storage requires the following steps:
 *
 *   1. define an ID for the service in the enum ps_storage_id
 *   2. define a new struct with the elements that should be stored for
 *      this service, with base class ps_storage_base_t
 *   3. add a new element to the config array found in storage.cpp
 *      in the form of:
 *   	  {ENUM_ID, {}, sizeof(struct)}
 *
 *  NOTE: we can't store less than 4 bytes, so even if you only want
 *    to store a byte, the element in the struct has to be defined as
 *    uint32_t
 */

/* define the maximum size for strings to be stored
 */
#define MAX_STRING_SIZE 32

/* Base class for storage structures
 */
struct ps_storage_base_t {

};

/* Enumeration used to identify the different storage structures
 */
enum ps_storage_id {
	PS_ID_POWER_SERVICE = 0,
	PS_ID_GENERAL_SERVICE = 1
};

/* Storage configuration struct
 *
 * This struct is used to define the configuration of the storage
 *
 *   * id 			is the enum <ps_storage_id> which defines
 *   				the type of storage structure
 *   * handle 		handle to the storage in FLASH
 *   * storage_size	size of the storage structure
 */
struct storage_config_t {
	ps_storage_id id;
	pstorage_handle_t handle;
	pstorage_size_t storage_size;
};

// POWER SERVICE /////////////////////////////////
// TODO: move to cs_PowerService.h

/* Struct used by the <PowerService> to store elements
 */
struct ps_power_service_t : ps_storage_base_t {
	uint32_t current_limit;
};

// GENERAL SERVICE ///////////////////////////////
// TODO: move to cs_GeneralService.h

/* Struct used by the <GeneralService> to store elements
 */
struct ps_general_service_t : ps_storage_base_t {
	char device_name[MAX_STRING_SIZE];
	char room[MAX_STRING_SIZE];
	char device_type[MAX_STRING_SIZE];
	uint32_t floor;
};

///////////////////////////////////////////////////

/* Class to store items persistently in FLASH
 *
 * Singleton class, can only exist once.
 *
 * This class provides functions to initialize, clear, write and read
 * persistent memory (FLASH).
 *
 * The class also provides static helper functions to read and write
 * values from/to the storage structs of the services. They do not operate
 * on the Storage itself, just work with the provided parameters. As such
 * they can be used without unnecessarily instantiating the storage instance.
 */
class Storage {

public:
	/* Returns the singleton instance of this class
	 */
	static Storage& getInstance() {
		static Storage instance;
		return instance;
	}

	/* Get the handle to the persistent memory for the given storage ID.
	 *
	 * @storageID the enum defining the storage struct which should be
	 *   accessed
	 *
	 * @handle returns the handle to the persistent memory where the
	 *   requested struct is stored, subsequent calls to read and write
	 *   from the storage will take this parameter as input
	 *
	 * @return true if operation successful, false otherwise
	 */
	bool getHandle(ps_storage_id storageID, pstorage_handle_t& handle);

	/* Clears the block for the given handle
	 *
	 * @handle the handle to the persistent memory which was obtained by
	 *   the <getHandle> function
	 *
	 * @storageID the enum defining the storage struct type, used to
	 *   get the size of the struct
	 */
	void clearBlock(pstorage_handle_t handle, ps_storage_id storageID);

	/* Get the struct stored in persistent memory at the position defined by
	 * the handle
	 *
	 * @handle the handle to the persistent memory where the struct is stored.
	 *   obtain the hanlde with <getHandle>
	 *
	 * @item pointer to the structure where the data from the persistent memory
	 *   should be copied into
	 *
	 * @size size of the structure (usually sizeof(struct))
	 */
	void getStruct(pstorage_handle_t handle, ps_storage_base_t* item, uint16_t size);

	/* Write the struct to persistent memory at the position defined by the handle
	 *
	 * @handle the handle to the persistent memory where the struct should be stored.
	 *   obtain the hanlde with <getHandle>
	 *
	 * @item pointer to the structure which data should be stored into persitent memory
	 *
	 * @size size of the structure (usually sizeof(struct))
	 */
	void setStruct(pstorage_handle_t handle, ps_storage_base_t* item, uint16_t size);

	////////////////////////////////////////////////////////////////////////////////////////
	// helper functions
	////////////////////////////////////////////////////////////////////////////////////////

	/* Helper function to convert std::string to char array
	 *
	 * @value the input string
	 *
	 * @target pointer to the output char array
	 */
	static void setString(std::string value, char* target);

	/* Helper function to get std::string from char array in the struct
	 *
	 * @value pointer the input char array
	 *
	 * @target the output string
	 *
	 * @default_value the default string to be used if no valid string found
	 *   in the char array
	 *
	 * In order to show that the field of the struct is unassigned, we use
	 * the fact that the last byte of the char array is set to FF.
	 * To show that the char array is empty, we fill it with 0.
	 *
	 * If the value read from the char array is empty or unassigned the default
	 * value will be returned instead
	 */
	static void getString(char* value, std::string& target, std::string default_value);

	/* Helper function to set a byte in the field of a struct
	 *
	 * @value the byte value to be copied to the struct
	 *
	 * @target pointer the field in the struct where the value should be set
	 *
	 * To show that a valid value was set, the last 3 bytes of the field
	 * are set to 0
	 */
	static void setUint8(uint8_t value, uint32_t& target);

	/* Helper function to read a byte from the field of a struct
	 *
	 * @value the field of the struct which should be read
	 *
	 * @target pointer to the byte where the value is returned
	 *
	 * @default_value the default value if the field of the struct is empty
	 *
	 * In order to show that the field of the struct is empty (or unassigned)
	 * we use the fact that the last byte of the uint32_t field is set to FF.
	 * If a value is stored, that byte will be set to 0 to show that the field
	 * is assigned and that a valid value can be read.
	 *
	 * If the field is unassigned, the default value will be returned instead
	 */
	static void getUint8(uint32_t value, uint8_t& target, uint8_t default_value);

	/* Helper function to set a short (uint16_t) in the field of a struct
	 *
	 * @value the value to be copied to the struct
	 *
	 * @target pointer to the field in the struct where the value should be set
	 *
	 * To show that a valid value was set, the last 2 bytes of the field
	 * are set to 0
	 */
	static void setUint16(uint16_t value, uint32_t& target);

	/* Helper function to read a short (uint16_t) from the field of a struct
	 *
	 * @value the field of the struct which should be read
	 *
	 * @target pointer the uint16_t varaible where the value is returned
	 *
	 * @default_value the default value if the field of the struct is empty
	 *
	 * In order to show that the field of the struct is empty (or unassigned)
	 * we use the fact that the last byte of the uint32_t field is set to FF.
	 * If a value is stored, that byte will be set to 0 to show that the field
	 * is assigned and that a valid value can be read.
	 *
	 * If the field is unassigned, the default value will be returned instead
	 */
	static void getUint16(uint32_t value, uint16_t& target, uint16_t default_value);

	/* Helper function to set an integer (uint32_t) in the field of a struct
	 *
	 * @value the value to be copied to the struct
	 *
	 * @target pointer to the field in the struct where the value should be set
	 *
	 * To show that a valid value was set, only values up to 2^32-2 (INT_MAX -1)
	 * can be stored, while the value 2^32-1 (INT_MAX) will be used to show that
	 * it is unassigned
	 */
	static void setUint32(uint32_t value, uint32_t& target);

	/* Helper function to read an integer (uint32_t) from the field of a struct
	 *
	 * @value the field of the struct which should be read
	 *
	 * @target pointer the uint16_t varaible where the value is returned
	 *
	 * @default_value the default value if the field of the struct is empty
	 *
	 * To show that a valid value was set, only values up to 2^32-2 (INT_MAX -1)
	 * can be stored, while the value 2^32-1 (INT_MAX) will be used to show that
	 * it is unassigned
	 *
	 * If the field is unassigned, the default value will be returned instead
	 */
	static void getUint32(uint32_t value, uint32_t& target, uint32_t default_value);

private:
	/* Default constructor
	 *
	 * Private because of the singleton class
	 *
	 * The constructor initializes the persistent memory blocks based on the
	 * configuration array.
	 */
	Storage();

	/* Initialize blocks in persistent memory
	 *
	 * @size size of the block to be initialized
	 *
	 * @handle pointer to the handle which points to the persistent memory that
	 *   was initialized
	 */
	void initBlocks(pstorage_size_t size, pstorage_handle_t& handle);

	/* Helper function to obtain the config for the given storage ID
	 *
	 * @storageID enum which defines the storage struct for which the
	 *   config should be obtained
	 *
	 * @return pointer to the config defined for the given enum if found,
	 *         null otherwise
	 */
	storage_config_t* getStorageConfig(ps_storage_id storageID);
};
