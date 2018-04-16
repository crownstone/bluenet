/*
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 24 Nov., 2014
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */
#pragma once

#include <string>

#include <ble/cs_Nordic.h>

#include "util/cs_Utils.h"

#include "structs/cs_TrackDevices.h"
#include "structs/cs_ScheduleEntries.h"

#include "structs/buffer/cs_CircularBuffer.h"
#include <events/cs_EventListener.h>

#if NORDIC_SDK_VERSION<=11
extern "C" {
	// the authors of the Nordic pstorage.h file forgot to include extern "C" wrappers
//	#include "pstorage_platform.h"
	#include "pstorage.h"
}
#else
	#include "fstorage.h"
#endif
/** enable additional debug output */
//#define PRINT_ITEMS

/**
 * We are only storing data to the persistent storage in form of
 * structs, every value that needs to be stored has to be encapsulated
 * inside a struct with base class ps_storage_base_t. Each service that
 * needs to store data gets a struct which it takes care of. Values are
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

	//! Current threshold in mA
	uint32_t currentThreshold;

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

	// [18.07.16] enabling and disabling functionality at run time will mess
	// up with enabling/disabling through compiler flags. i.e. if one flag was
	// set through the command at run time, all flags are initialized with the
	// current compiler flags. if next run, the compiler flags are changed, the
	// change will not take any effect. to reduce this during development, use
	// instead separate variables for each flag. this will be a waste of space
	// but it lead to less confusion.
	// nevertheless, if a flag is set at runtime through the command, changing the
	// compiler flag afterwards will have no influence anymore
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
	int32_t voltageZero;
	int32_t currentZero;
	int32_t powerZero;

	// see comment at struct for flags above
	uint32_t meshEnabled;
	uint32_t encryptionEnabled;
	uint32_t iBeaconEnabled;
	uint32_t scannerEnabled;
	uint32_t continuousPowerSamplerEnabled;
	uint32_t trackerEnabled;
	uint32_t defaultOff;

	uint32_t powerZeroAvgWindow;

	uint32_t meshAccessAddress;

	uint32_t pwmInterval;

	uint32_t currentThresholdPwm;

	float pwmTempVoltageThresholdUp;
	float pwmTempVoltageThresholdDown;

	uint32_t pwmAllowed; //! Flag indicating whether this crownstone is marked as dimmable.
	uint32_t switchLocked; //! Flag indicating whether this crownstone is allowed to change switch state.
	uint32_t switchCraftEnabled; // Flag indicating whether this crownstone has switchcraft enabled.
};

//! size of one block in eeprom can't be bigger than 1024 bytes. => create a new struct
//STATIC_ASSERT(sizeof(ps_configuration_t) <= 0x1000);
STATIC_ASSERT(sizeof(ps_configuration_t) <= PSTORAGE_FLASH_PAGE_SIZE);

/** Event handler for softdevice events. in particular, listen for
 *  NRF_EVT_RADIO_SESSION_CLOSED event to resume storage requests
 */
void storage_sys_evt_handler(uint32_t evt);

/** Struct holding a storage request. If meshing is enabled, we need
 *  to pause meshing on a storage request and wait until the radio is
 *  closed before we can update the storage. so we need to buffer the
 *  storage requests.
 */
struct __attribute__((__packed__)) buffer_element_t {
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

	void resumeRequests();

	// Handle events as EventListener
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

	// Keeps up how many writes are pending in pstorage.
	uint8_t _pending;

};
