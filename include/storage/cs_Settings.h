/*
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 11, 2015
 * License: LGPLv3+
 */
#pragma once

#include <events/cs_EventTypes.h>
#include <drivers/cs_Storage.h>
#include <structs/cs_StreamBuffer.h>
#include <protocol/cs_ConfigTypes.h>
#include <protocol/cs_ErrorCodes.h>
#include <cfg/cs_Boards.h>

/**
 * Load settings from and save settings to persistent storage.
 */
class Settings {

private:
	Settings();

	//! This class is singleton, deny implementation
	Settings(Settings const&);
	//! This class is singleton, deny implementation
	void operator=(Settings const &);

public:
	static Settings& getInstance() {
		static Settings instance;
		return instance;
	}

	void init(boards_config_t* boardsConfig);

	bool isInitialized() {
		return _initialized;
	}

	/** Read the configuration from the buffer and store in working memory.
	 *  If persistent is true, also store in FLASH
	 */
	ERR_CODE writeToStorage(uint8_t type, uint8_t* payload, uint16_t length, bool persistent = true);

	/** Read the configuration from storage and write to streambuffer (to be read from characteristic)
	 */
	ERR_CODE readFromStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer);

	/** Return the struct containing the configuration values in current working memory
	 */
//	ps_configuration_t& getConfig();

	/** Get a handle to the persistent storage struct and load it from FLASH into working memory.
	 *
	 * Persistent storage is implemented in FLASH. Just as with SSDs, it is important to realize that
	 * writing less than a minimal block strains the memory just as much as flashing the entire block.
	 * Hence, there is an entire struct that can be filled and flashed at once.
	 */
	void loadPersistentStorage();

	/** Save the whole configuration struct (working memory) to FLASH.
	 */
	void savePersistentStorage();

	/** Save the whole iBeacon struct to FLASH
	 */
	void saveIBeaconPersistent();

	/** Retrieve the Bluetooth name from the object representing the BLE stack.
	 *
	 * @return name of the device
	 */
	std::string & getBLEName();

	/** Write the Bluetooth name to the object representing the BLE stack.
	 *
	 * This updates the Bluetooth name immediately, however, it does not update the name persistently. It
	 * has to be written to FLASH in that case.
	 */
	void setBLEName(const std::string &name, bool persistent = true);

	bool isSet(uint8_t type);
	bool updateFlag(uint8_t type, bool value, bool persistent);

	void factoryReset(uint32_t resetCode);

	ERR_CODE get(uint8_t type, void* target, bool getDefaultValue = false);
	ERR_CODE get(uint8_t type, void* target, uint16_t& size, bool getDefaultValue = false);
	ERR_CODE set(uint8_t type, void* target, bool persistent = false, uint16_t size = 0);

	uint16_t getSettingsItemSize(uint8_t type);

protected:

	bool _initialized;

	//! handle to storage (required to write to and read from FLASH)
	pstorage_handle_t _storageHandle;

	//! struct that storage object understands
	ps_configuration_t _storageStruct;

	//! non-persistent configuration options
	std::string _wifiSettings;

	Storage* _storage;

	boards_config_t* _boardsConfig;

	ERR_CODE verify(uint8_t type, uint8_t* payload, uint8_t length);

	bool readFlag(uint8_t type, bool& value);
//	void initFlags();

	/**
	 * Helper functions to write single item from the configuration struct to storage (FLASH).
	 */
	void savePersistentStorageItem(uint32_t* item);
	void savePersistentStorageItem(int32_t* item);
	void savePersistentStorageItem(uint8_t* item, uint8_t size = 1);

};


