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

	/** Write the buffer to flash or ram.
	 */
	ERR_CODE writeToStorage(CS_TYPE type, uint8_t* data, size16_t size, PersistenceMode mode);

	/** Read the configuration from storage and write to streambuffer (to be read from characteristic)
	 */
	ERR_CODE readFromStorage(CS_TYPE type, StreamBuffer<uint8_t>* streamBuffer);

	bool isSet(CS_TYPE type);

	bool updateFlag(CS_TYPE type, bool value, PersistenceMode mode);

	void factoryReset(uint32_t resetCode);

	ERR_CODE get(const CS_TYPE type, void* data, const PersistenceMode mode);

	/*
	 * @param type           One of the types from the CS_TYPE enumeration class.
	 * @param data           Pointer to where we have to write the data.
	 * @param size[inout]    If size is non-zero, it will indicate maximum size available. Afterwards it will have
	 *                       the size of the data.
	 * @param mode           Indicates to get data from RAM, FLASH, PRECOMPILER_DEFAULT, or a combination of this.
	 */
	ERR_CODE get(const CS_TYPE type, void* data, size16_t & size, const PersistenceMode mode);

	ERR_CODE set(CS_TYPE type, void* data, size16_t size, PersistenceMode mode);

	//size16_t getStateItemSize(CS_TYPE type);

	void setNotify(CS_TYPE type, bool enable);

	bool isNotifying(CS_TYPE type);
	
	void disableNotifications();

protected:

	bool _initialized;

	Storage* _storage;

	boards_config_t* _boardsConfig;

	ERR_CODE verify(CS_TYPE type, uint8_t* payload, uint8_t length);

	bool readFlag(CS_TYPE type, bool& value);

	ERR_CODE storeInRam(const st_file_data_t & data);

	ERR_CODE loadFromRam(st_file_data_t & data);

	std::vector<CS_TYPE> _notifyingStates;

	std::vector<st_file_data_t> _not_persistent;
};


