/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 11, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <cfg/cs_Boards.h>
#include <common/cs_BaseClass.h>
#include <common/cs_Types.h>
#include <drivers/cs_Storage.h>
#include <protocol/cs_ErrorCodes.h>
#include <structs/cs_StreamBuffer.h>

enum class OperationMode {
	OPERATION_MODE_SETUP                       = 0x00,
	OPERATION_MODE_DFU                         = 0x01,
	OPERATION_MODE_FACTORY_RESET               = 0x02,
	OPERATION_MODE_NORMAL                      = 0x10,
	OPERATION_MODE_UNINITIALIZED               = 0xFF,
};

constexpr const char* TypeName(OperationMode const & mode) {
    switch(mode) {
	case OperationMode::OPERATION_MODE_SETUP:
	    return "OPERATION_MODE_SETUP";
	case OperationMode::OPERATION_MODE_DFU:
	    return "OPERATION_MODE_DFU";
	case OperationMode::OPERATION_MODE_FACTORY_RESET:
	    return "OPERATION_MODE_FACTORY_RESET";
	case OperationMode::OPERATION_MODE_NORMAL:
	    return "OPERATION_MODE_NORMAL";
	case OperationMode::OPERATION_MODE_UNINITIALIZED:
	    return "OPERATION_MODE_UNINITIALIZED";
    }
    // should never be reached
    return "Mode does not exist!";
}

constexpr int ValidMode(OperationMode const & mode) {
    switch(mode) {
	case OperationMode::OPERATION_MODE_SETUP:
	case OperationMode::OPERATION_MODE_DFU:
	case OperationMode::OPERATION_MODE_FACTORY_RESET:
	case OperationMode::OPERATION_MODE_NORMAL:
	case OperationMode::OPERATION_MODE_UNINITIALIZED:
	    return 1;
    }
    return 0;
}

#define FACTORY_RESET_STATE_NORMAL 0
#define FACTORY_RESET_STATE_LOWTX  1
#define FACTORY_RESET_STATE_RESET  2

/**
 * Load settings from and save settings to FLASH (persistent storage) and or RAM. If we save values to FLASH we
 * should only do that if the values are different from the FIRMWARE_DEFAULT. 
 *
 * If we read vales from FLASH and they are not written in FLASH, we should get them from the FIRMWARE_DEFAULT instead.
 * 
 * To prevent a round-trip to search for records in FLASH, we always will cache the records in RAM. So, if a record
 * is present in RAM we will use that value, we will not read it from FLASH. 
 *
 * Default FLASH read procedure:
 *   1. Read RAM
 *     - if present, return RAM value.
 *   2. Read FLASH
 *     - if present, copy to RAM, return RAM value.
 *   3. Read FIRMWARE_DEFAULT
 *     - if present, copy to RAM, return RAM value
 *     - if not present, should be impossible: compile-time error please!
 *
 * Default FLASH write procedure (outdated):
 *   1. Compare FIRMWARE_DEFAULT
 *     - if same, return success
 *   2. Compare FLASH
 *     - if same, return success
 *   3. Write to FLASH and copy to RAM
 *
 * Note. The above has a bug. When a value has been written to FLASH and then the FIRMWARE_DEFAULT needs to be 
 * written the above protocol fails to write FIRMWARE_DEFAULT values. 
 *
 * Default FLASH write procedure:
 *   1. Compare with RAM
 *     - if same, return success
 *   2. Write to FLASH
 *
 * We assume here that a value even if it is the same as FIRMWARE_DEFAULT it will still be written. To remove a 
 * value should be a deliberate action.
 *
 * Default RAM read procedure:
 *   1. Read RAM
 *     - if present, return RAM value
 *     - if not present, return failure
 *
 * Default RAM write procedure:
 *   1. Write to RAM (no comparison, just writing is faster than first checking if it is the same)
 *
 * It would be cumbersome to decide per item if it needs to be written to or read from FLASH or from RAM. So, we have
 * an overarching read and write procedure
 * 
 * Default read procedure:
 *   1. Obtain default location
 *     a. Case default location is RAM
 *       - READ RAM
 *     b. Case default location is FLASH
 *       - READ FLASH
 *
 * Default write procedure:
 *   1. Obtain default location
 *     a. Case default location is RAM
 *       - WRITE RAM
 *     b. Case default location is FLASH
 *       - WRITE FLASH
 *
 * The above means quite a sophisticated setup in reading/writing, so it is encapsulated in a dedicated 
 * PersistenceMode, which is called STRATEGY1.
 */
class State: public BaseClass<> {
public:
	/** Get a reference to the State object.
	 */
	static State& getInstance() {
		static State instance;
		return instance;
	}

	/** Initialize the State object with the board configuration.
	 *
	 * @param[in] boardsConfig  Board configuration (pin layout, etc.).
	 */
	void init(boards_config_t* boardsConfig);

	/** Write the buffer to flash or ram.
	 */
	cs_ret_code_t writeToStorage(CS_TYPE type, uint8_t* data, size16_t size, PersistenceMode mode);

	/** Read the configuration from storage and write to streambuffer (to be read from characteristic)
	 */
	cs_ret_code_t readFromStorage(CS_TYPE type, StreamBuffer<uint8_t>* streamBuffer);

	/**
	 * Shorthand for get(data, PersistenceMode::STRATEGY1) for boolean data types.
	 */
	bool isSet(CS_TYPE type);

	/**
	 */
	void factoryReset(uint32_t resetCode);

	cs_ret_code_t get(const CS_TYPE type, void* data, const PersistenceMode mode);

	/* The function gets an item from memory.
	 *
	 * It is most "clean" to not use thousands of arguments, but have a struct as argument. Yes, this means that 
	 * the person calling this function has to write the arguments into a struct. However, it makes it much 
	 * easier to add an additional argument.
	 *
	 * @param[in] type          One of the types from the CS_TYPE enumeration class.
	 * @param[out] data         Pointer to where we have to write the data.
	 * @param[in,out] size      If size is non-zero, it will indicate maximum size available. Afterwards it will have
	 *                          the size of the data.
	 * @param[in] mode          Indicates to get data from RAM, FLASH, FIRMWARE_DEFAULT, or a combination of this.
	 *
	 * @deprecated              Use get(st_file_data_t, mode).
	 */
	cs_ret_code_t get(const CS_TYPE type, void* data, size16_t & size, const PersistenceMode mode);

	/* The function gets an item from memory.
	 *
	 * @param[in,out] data      Data struct (contains type, pointer to data, and size).
	 * @param[in] mode          Indicates to get data from RAM, FLASH, FIRMWARE_DEFAULT, or a combination of this.
	 */
	cs_ret_code_t get(st_file_data_t & data, const PersistenceMode mode);

	cs_ret_code_t set(CS_TYPE type, void* data, size16_t size, PersistenceMode mode);

	cs_ret_code_t remove(CS_TYPE type, const PersistenceMode mode);
	
	void setNotify(CS_TYPE type, bool enable);

	bool isNotifying(CS_TYPE type);
	
	void disableNotifications();

protected:

	Storage* _storage;

	boards_config_t* _boardsConfig;

	cs_ret_code_t verify(CS_TYPE type, uint8_t* payload, uint8_t length);

	bool readFlag(CS_TYPE type, bool& value);

	cs_ret_code_t storeInRam(const st_file_data_t & data);

	cs_ret_code_t loadFromRam(st_file_data_t & data);

	cs_ret_code_t storeInRam(const st_file_data_t & data, size16_t & index_in_ram);

	std::vector<CS_TYPE> _notifyingStates;

	std::vector<st_file_data_t> _data_in_ram;

private:

	//! State constructor, singleton, thus made private
	State();
	
	//! State copy constructor, singleton, thus made private
	State(State const&);

	//! Aassignment operator, singleton, thus made private
	void operator=(State const &);
};
