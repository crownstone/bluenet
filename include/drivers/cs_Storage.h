/*
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 24 Nov., 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */
#pragma once

#include <string>

#include <ble/cs_Nordic.h>

#include "drivers/cs_Serial.h"
#include "util/cs_Utils.h"

#include "structs/cs_TrackDevices.h"
#include "structs/cs_ScheduleEntries.h"

#include "structs/buffer/cs_CircularBuffer.h"

extern "C" {
	// the authors of the Nordic pstorage.h file forgot to include extern "C" wrappers
//	#include "pstorage_platform.h"
	#include "pstorage.h"
}

/** enable additional debug output */
//#define PRINT_ITEMS

/**
 * We are only storing data to the persistent storage in form of
 * structs, every value that needs to be stored has to be encapsulated
 * inside a struct with base class ps_storage_base_t. Each service that
 * needs to store data get's a struct which it takes care of. Values are
 * written and read to that struct, which is a local copy of the data
 * in memory. Changes to values are only kept once that struct is
 * written to memory.
 *
 * Adding a new element to the storage requires the following steps:
 *
 *   1. define an ID for the service in the enum ps_storage_id
 *   2. define a new struct with the elements that should be stored for
 *      this service, with base class ps_storage_base_t
 *   3. add a new element to the config array found in storage.cpp
 *      in the form of:
 *   	  {ENUM_ID, {}, sizeof(struct)}
 *   4. Increase PSTORAGE_MAX_APPLICATIONS in pstorage_platform.h
 *      by one
 *
 *  NOTE: we can't store less than 4 bytes, so even if you only want
 *    to store a byte, the element in the struct has to be defined as
 *    uint32_t
 */

/** Base class for storage structures
 */
struct ps_storage_base_t {

};

/** Enumeration used to identify the different storage structures
 */
enum ps_storage_id {
	//! storage for configuration items
	PS_ID_CONFIGURATION = 0,
	//! storage for the indoor localisation service
	PS_ID_GENERAL = 1,
	//! state variables
	PS_ID_STATE = 2,
	//! number of elements
	PS_ID_TYPES
};

/** Storage configuration struct
 *
 * This struct is used to define the configuration of the storage
 */
struct storage_config_t {
	//! enum <ps_storage_id> defines the type of storage structure
	ps_storage_id id;
	//! handle to the storage in FLASH
	pstorage_handle_t handle;
	//! size of the storage structure
	pstorage_size_t storage_size;
};

// Configuration ///////////////////////////////

/** Struct used by the Configuration to store elements.
 */
struct ps_configuration_t : ps_storage_base_t {
	//! device name
	char device_name[MAX_STRING_STORAGE_SIZE+1];
	//! room name
	char room[MAX_STRING_STORAGE_SIZE+1];
	//! device type
	char device_type[MAX_STRING_STORAGE_SIZE+1];
	//! floor level
	uint32_t floor;
	//! beacon
	struct __attribute__((__packed__)) ps_beacon_t {
		uint32_t major;
		uint32_t minor;
		ble_uuid128_t uuid;
		int32_t txPower;
	} beacon;

	//! current limit value
	uint32_t currentLimit;

	//! nearby timeout used for device tracking
	uint32_t nearbyTimeout;

	//! Transmission power in dBm, see ble_gap.h
	int32_t txPower;

	//! Advertisement interval in units of 0.625 ms
	uint32_t advInterval;

	//! passkey used for bonding
	uint8_t passkey[BLE_GAP_PASSKEY_LEN];

	//! Minimum temperature of environment
	int32_t minEnvTemp;

	//! Maximum temperature of environment
	int32_t maxEnvTemp;

	//! Scan duration in ms
	uint32_t scanDuration;

	//! Time in ms, before sending the scan results over the mesh
	uint32_t scanSendDelay;

	//! Time in ms to wait before scanning, after sending the scan results
	uint32_t scanBreakDuration;

	//! Time (in ms) to wait before booting after reset
	uint32_t bootDelay;

	//! The temperature (in C) at which the PWM and relay are switched off
	int32_t maxChipTemp;

	//! current scan filter
	uint32_t scanFilter;

	//! Filtered out devices are still sent once every N scan intervals
	//! Set to 0 to not send them ever
	uint32_t scanFilterSendFraction;

	// interval duration between two calls to sample for power
	uint32_t samplingInterval;

	// how long to sample for power
	uint32_t samplingTime;

	// todo: might need to move this to the state variables for wear leveling
	union {
		struct {
			bool flagsUninitialized : 1;
			bool meshDisabled : 1;
			bool encryptionDisabled : 1;
			bool iBeaconDisabled : 1;
			bool scannerDisabled : 1;
			bool continuousPowerSamplerDisabled : 1;
			bool trackerDisabled : 1;
			bool defaultOff: 1;
		} flagsBit;
		// dummy to force enableFlags struct to be of size uint32_t;
		uint32_t flags;
	};

	uint32_t crownstoneId;

	// storing keys used for encryption
	struct {
		uint8_t owner[ENCYRPTION_KEY_LENGTH];
		uint8_t member[ENCYRPTION_KEY_LENGTH];
		uint8_t guest[ENCYRPTION_KEY_LENGTH];
	} encryptionKeys;

	uint32_t adcBurstSampleRate;

	uint32_t powerSampleBurstInterval;

	uint32_t powerSampleContInterval;

	uint32_t adcContSampleRate;

	uint32_t scanInterval;

	uint32_t scanWindow;

	uint32_t relayHighDuration;

	int32_t lowTxPower;

	float voltageMultiplier;
	float currentMultiplier;
	float voltageZero;
	float currentZero;
	float powerZero;
};

//! size of one block in eeprom can't be bigger than 1024 bytes. => create a new struct
STATIC_ASSERT(sizeof(ps_configuration_t) <= 1024);

/** Event handler for softdevice events. in particular, listen for
 *  NRF_EVT_RADIO_SESSION_CLOSED event to resume storage requests
 */
void storage_sys_evt_handler(uint32_t evt);

/** Struct holding a storage request. If meshing is enabled, we need
 *  to pause meshing on a storage request and wait until the radio is
 *  closed before we can update the storage. so we need to buffer the
 *  storage requests.
 */
struct buffer_element_t {
	uint8_t* data;
	uint16_t dataSize;
	uint16_t storageOffset;
	pstorage_handle_t storageHandle;
};

/** Class to store items persistently in FLASH
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
class Storage : EventListener {

public:
	/** Returns the singleton instance of this class
	 *
	 * @return static instance of Storage class
	 */
	static Storage& getInstance() {
		static Storage instance;
		return instance;
	}

	void init();

	bool isInitialized() {
		return _initialized;
	}

	/** Get the handle to the persistent memory for the given storage ID.
	 * @storageID the enum defining the storage struct which should be
	 *   accessed
	 * @handle returns the handle to the persistent memory where the
	 *   requested struct is stored, subsequent calls to read and write
	 *   from the storage will take this parameter as input
	 *
	 * Get the handle to persistent memory for a given storage ID.
	 *
	 * @return true if operation successful, false otherwise
	 */
	bool getHandle(ps_storage_id storageID, pstorage_handle_t& handle);

	/** Clears the block for the given handle
	 * @handle the handle to the persistent memory which was obtained by
	 *   the <getHandle> function
	 * @storageID the enum defining the storage struct type, used to
	 *   get the size of the struct
	 */
	void clearStorage(ps_storage_id storageID);

	/** Get the struct stored in persistent memory at the position defined by the handle
	 * @handle the handle to the persistent memory where the struct is stored.
	 *   obtain the handle with <getHandle>
	 * @item pointer to the structure where the data from the persistent memory
	 *   should be copied into
	 * @size size of the structure (usually sizeof(struct))
	 */
	void readStorage(pstorage_handle_t handle, ps_storage_base_t* item, uint16_t size);

	/** Write the struct to persistent memory at the position defined by the handle
	 * @handle the handle to the persistent memory where the struct should be stored.
	 *   obtain the hanlde with <getHandle>
	 * @item pointer to the structure which data should be stored into persitent memory
	 * @size size of the structure (usually sizeof(struct))
	 */
	void writeStorage(pstorage_handle_t handle, ps_storage_base_t* item, uint16_t size);

	/** Read a single item (variable) from persistent memory at the position defined by the handle
	 * and the offset
	 * @handle the handle to the persistent memory where the struct is stored.
	 *   obtain the handle with <getHandle>
	 * @item pointer to the structure where the data from the persistent memory
	 *   should be copied into
	 * @size size of the item (eg. 4 for integer, 1 for a byte, 8 for a byte array of length 8, etc)
	 * @offset the offset in bytes at which the item should be stored. (the offset is relative to the
	 *   beginning of the block defined by the handle)
	 */
	void readItem(pstorage_handle_t handle, pstorage_size_t offset, uint8_t* item, uint16_t size);

	/** Write a single item (variable) to persistent memory at the position defined by the handle
	 * and the offset
	 * @handle the handle to the persistent memory where the struct is stored.
	 *   obtain the handle with <getHandle>
	 * @item pointer to the structure where the data from the persistent memory
	 *   should be copied into
	 * @size size of the item (eg. 4 for integer, 1 for a byte, 8 for a byte array of length 8, etc)
	 * @offset the offset in bytes at which the item should be stored. (the offset is relative to the
	 *   beginning of the block defined by the handle)
	 */
	void writeItem(pstorage_handle_t handle, pstorage_size_t offset, uint8_t* item, uint16_t size);

	////////////////////////////////////////////////////////////////////////////////////////
	//! helper functions
	////////////////////////////////////////////////////////////////////////////////////////

	/** Helper function to calculate the offset of a variable inside a storage struct
	 * @storage pointer to the storage struct where the variable is stored. (storage struct is a
	 * 1 to 1 representation of the FLASH memory)
	 * @variable pointer to the variable inside the struct
	 * @return returns the offset in bytes
	 */
	static pstorage_size_t getOffset(ps_storage_base_t* storage, uint8_t* variable);

	/** Helper function to convert std::string to char array
	 * @value the input string
	 * @target pointer to the output char array
	 */
	static void setString(std::string value, char* target);
	static void setString(const char* value, uint16_t length, char* target);

	/** Helper function to get std::string from char array in the struct
	 * @value pointer the input char array
	 * @target the output string
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
//	static void getString(char* value, std::string& target, std::string default_value);
	static void getString(char* value, char* target, char* default_value, uint16_t& size);

	/** Helper function to set a byte in the field of a struct
	 * @value the byte value to be copied to the struct
	 * @target pointer the field in the struct where the value should be set
	 *
	 * To show that a valid value was set, the last 3 bytes of the field
	 * are set to 0
	 */
	static void setUint8(uint8_t value, uint32_t& target);

	/** Helper function to read a byte from the field of a struct
	 * @value the field of the struct which should be read
	 * @target pointer to the byte where the value is returned
	 * @default_value the default value if the field of the struct is empty
	 *
	 * In order to show that the field of the struct is empty (or unassigned)
	 * we use the fact that the last byte of the uint32_t field is set to FF.
	 * If a value is stored, that byte will be set to 0 to show that the field
	 * is assigned and that a valid value can be read.
	 *
	 * If the field is unassigned, the default value will be returned instead
	 */
	static void getUint8(uint32_t value, uint8_t* target, uint8_t default_value);

	/** Helper function to set a short (uint16_t) in the field of a struct
	 * @value the value to be copied to the struct
	 * @target pointer to the field in the struct where the value should be set
	 *
	 * To show that a valid value was set, the last 2 bytes of the field
	 * are set to 0
	 */
	static void setUint16(uint16_t value, uint32_t& target);

	/** Helper function to read a short (uint16_t) from the field of a struct
	 * @value the field of the struct which should be read
	 * @target pointer the uint16_t varaible where the value is returned
	 * @default_value the default value if the field of the struct is empty
	 *
	 * In order to show that the field of the struct is empty (or unassigned)
	 * we use the fact that the last byte of the uint32_t field is set to FF.
	 * If a value is stored, that byte will be set to 0 to show that the field
	 * is assigned and that a valid value can be read.
	 *
	 * If the field is unassigned, the default value will be returned instead
	 */
	static void getUint16(uint32_t value, uint16_t* target, uint16_t default_value);

	/** Helper function to set an integer (uint32_t) in the field of a struct
	 * @value the value to be copied to the struct
	 * @target pointer to the field in the struct where the value should be set
	 *
	 * To show that a valid value was set, only values up to 2^32-2 (INT_MAX -1)
	 * can be stored, while the value 2^32-1 (INT_MAX) will be used to show that
	 * it is unassigned
	 */
	static void setUint32(uint32_t value, uint32_t& target);

	/** Helper function to read an integer (uint32_t) from the field of a struct
	 * @value the field of the struct which should be read
	 * @target pointer the uint16_t varaible where the value is returned
	 * @default_value the default value if the field of the struct is empty
	 *
	 * To show that a valid value was set, only values up to 2^32-2 (INT_MAX -1)
	 * can be stored, while the value 2^32-1 (INT_MAX) will be used to show that
	 * it is unassigned
	 *
	 * If the field is unassigned, the default value will be returned instead
	 */
	static void getUint32(uint32_t value, uint32_t* target, uint32_t default_value);

	/** Helper function to set a signed byte in the field of a struct
	 * @value the byte value to be copied to the struct
	 * @target pointer the field in the struct where the value should be set
	 *
	 * To show that a valid value was set, the last 3 bytes of the field
	 * are set to 0
	 */
	static void setInt8(int8_t value, int32_t& target);

	/** Helper function to read a signed byte from the field of a struct
	 * @value the field of the struct which should be read
	 * @target pointer to the byte where the value is returned
	 * @default_value the default value if the field of the struct is empty
	 *
	 * In order to show that the field of the struct is empty (or unassigned)
	 * we use the fact that the last byte of the uint32_t field is set to FF.
	 * If a value is stored, that byte will be set to 0 to show that the field
	 * is assigned and that a valid value can be read.
	 *
	 * If the field is unassigned, the default value will be returned instead
	 */
	static void getInt8(int32_t value, int8_t* target, int8_t default_value);

//	static void setDouble(double value, double& target);

//	static void getDouble(double value, double* target, double default_value);

	static void setFloat(float value, float& target);

	static void getFloat(float value, float* target, float default_value);

	/** Helper function to write/copy an array to the field of a struct
	 * @T primitive type, such as uint8_t
	 * @src pointer to the array to be written
	 * @dest pointer to the array field of the struct
	 * @length the number of elements in the source array. the destination
	 *   array needs to have space for at least length elements
	 *
	 * Copies the data contained in the src array to the destination
	 * array. It is not possible to write an array containing only FF
	 * to memory, since that is used to show that the array is
	 * uninitialized.
	 */
	template<typename T>
	static void setArray(T* src, T* dest, uint16_t length) {
		bool isUnassigned = true;
		uint8_t* ptr = (uint8_t*)src;
		for (int i = 0; i < length * sizeof(T); ++i) {
			if (ptr[i] != 0xFF) {
				isUnassigned = false;
				break;
			}
		}

		if (isUnassigned) {
			LOGe("cannot write all FF");
		} else {
			memcpy(dest, src, length * sizeof(T));
		}
	}

	/** Helper function to read an array from a field of a struct
	 * @T primitive type, such as uint8_t
	 * @src pointer to the array field of the struct
	 * @dest pointer to the destination array
	 * @default_value pointer to an array containing the default values
	 *   can be NULL pointer
	 * @length number of elements in the array (all arrays need to have
	 *   the same length!)
	 * @return returns true if an array was found in storage, false otherwise
	 *
	 * Checks the memory of the source array field. If it is all FF, that means
	 * the memory is unassigned. If an array is provided as default_value,
	 * that array will be copied to the destination array, if no default_value
	 * array is provided (NULL pointer), nothing happens and the destination
	 * array will remain as it was. Otherwise, the data from the source array
	 * field will be copied to the destination array
	 */
	template<typename T>
	static bool getArray(T* src, T* dest, T* default_value, uint16_t length) {

#ifdef PRINT_ITEMS
		_log(INFO, "raw value: \r\n");
		BLEutil::printArray((uint8_t*)src, length * sizeof(T));
#endif

		bool isUnassigned = true;
		uint8_t* ptr = (uint8_t*)src;
		for (int i = 0; i < length * sizeof(T); ++i) {
			if (ptr[i] != 0xFF) {
				isUnassigned = false;
				break;
			}
		}

		if (isUnassigned) {
			if (default_value != NULL) {
#ifdef PRINT_ITEMS
				LOGd("use default value");
#endif
				memcpy(dest, default_value, length * sizeof(T));
			} else {
				memset(dest, 0, length * sizeof(T));
			}
		} else {
			memcpy(dest, src, length * sizeof(T));
		}

		return !isUnassigned;
	}

	void resumeRequests();

	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

	void onUpdateDone();

private:

	/** Default constructor
	 *
	 * Private because of the singleton class
	 *
	 * The constructor initializes the persistent memory blocks based on the
	 * configuration array.
	 */
	Storage();

	/** Initialize blocks in persistent memory
	 * @size size of the block to be initialized
	 * @handle pointer to the handle which points to the persistent memory that
	 *   was initialized
	 */
	void initBlocks(pstorage_size_t size, pstorage_size_t count, pstorage_handle_t& handle);

	/** Helper function to obtain the config for the given storage ID
	 * @storageID enum which defines the storage struct for which the
	 *   config should be obtained
	 *
	 * @return pointer to the config defined for the given enum if found,
	 *         null otherwise
	 */
	storage_config_t* getStorageConfig(ps_storage_id storageID);

	bool _initialized;
	bool _scanning;

	CircularBuffer<buffer_element_t> writeBuffer;
	buffer_element_t buffer[STORAGE_REQUEST_BUFFER_SIZE];

	uint8_t _pending;

};
