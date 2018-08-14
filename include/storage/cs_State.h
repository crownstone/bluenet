/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 11, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cfg/cs_Boards.h>
#include <drivers/cs_Storage.h>
#include <events/cs_EventTypes.h>
#include <protocol/cs_ConfigTypes.h>
#include <protocol/cs_StateTypes.h>
#include <protocol/cs_ErrorCodes.h>
#include <structs/cs_StreamBuffer.h>

#define OPERATION_MODE_SETUP                       0x00
#define OPERATION_MODE_DFU                         0x01
#define OPERATION_MODE_FACTORY_RESET               0x02
#define OPERATION_MODE_NORMAL                      0x10

#define FACTORY_RESET_STATE_NORMAL 0
#define FACTORY_RESET_STATE_LOWTX  1
#define FACTORY_RESET_STATE_RESET  2


/**
 * Load settings from and save settings to persistent storage.
 */
class State {
private:
	State();
	State(State const&);
	void operator=(State const &);
public:
	static State& getInstance() {
		static State instance;
		return instance;
	}

	void init(boards_config_t* boardsConfig);

	bool isInitialized() {
		return _initialized;
	}

	/** Read the configuration from the buffer and store in working memory.
	 *  If persistent is true, also store in FLASH
	 */
	ERR_CODE writeToStorage(uint8_t type, uint8_t* data, size_t size, bool persistent = true);

	/** Read the configuration from storage and write to streambuffer (to be read from characteristic)
	 */
	ERR_CODE readFromStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer);

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

	ERR_CODE get(const uint8_t type, void* data, const bool getDefaultValue = false);

	ERR_CODE get(const uint8_t type, void* data, size_t & size, const bool getDefaultValue = false);

	ERR_CODE set(uint8_t type, void* data, size_t size, bool persistent);

	size_t getStateItemSize(uint8_t type);

	void setNotify(uint8_t type, bool enable);

	bool isNotifying(uint8_t type);
	
	void disableNotifications();

protected:

	bool _initialized;

	Storage* _storage;

	boards_config_t* _boardsConfig;

	ERR_CODE verify(uint8_t type, uint8_t* payload, uint8_t length);

	bool readFlag(uint8_t type, bool& value);

	std::vector<uint8_t> _notifyingStates;

	std::vector<st_file_data_t> _not_persistent;
};


