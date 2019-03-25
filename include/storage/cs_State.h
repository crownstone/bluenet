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

// TODO: what to do when for invalid operation mode?
// TODO: how can we use TYPIFY() as parameter type?
constexpr OperationMode getOperationMode(uint8_t mode) {
	OperationMode opMode = static_cast<OperationMode>(mode);
	if (ValidMode(opMode)) {
		return opMode;
	}
	return OperationMode::OPERATION_MODE_UNINITIALIZED;
}

#define FACTORY_RESET_STATE_NORMAL 0
#define FACTORY_RESET_STATE_LOWTX  1
#define FACTORY_RESET_STATE_RESET  2

/**
 * Stores state values in RAM and/or FLASH.
 *
 * Each state type always have a FIRMWARE_DEFAULT value.
 * Values that are set, always take precedence over FIRMWARE_DEFAULT.
 *
 * Values like chip temperature, that don't have to be stored over reboots should be stored in and read from RAM.
 * This complete persistence strategy is called RAM_OR_DEFAULT.
 *
 * Values like CONFIG_BOOT_DELAY should be known over reboots of the device. Moreover, they should also persist
 * over firmware updates. These values are stored in FLASH.
 * If the values are not changed, they should NOT be stored to FLASH. They can then immediately be read from the
 * FIRMWARE_DEFAULT. If these values are stored in FLASH, they always take precedence over FIRMWARE_DEFAULT values.
 * Values that are stored in FLASH will be cached in RAM, to prevent having to read from FLASH every time.
 * This complete persistence strategy is called CACHED_FLASH_OR_DEFAULT.
 *
 * If a new firmware has a new FIRMWARE_DEFAULT that should be enforced, we need to explicitly remove the value from
 * FLASH before updating the firmware.
 * For example, a new CONFIG_BOOT_DELAY may have to be enforced, or else it might go into an infinite reboot loop.
 *
 * For each state type it is defined whether it should be stored in RAM or FLASH.
 */

/**
 * Load settings from and save settings to FLASH (persistent storage) and or RAM. If we save values to FLASH we
 * should only do that if the values are different from the FIRMWARE_DEFAULT.
 *
 * If we read values from FLASH and they are not written in FLASH, we should get them from the FIRMWARE_DEFAULT instead.
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
	 * @param[in] boardsConfig    Board configuration (pin layout, etc.).
	 */
	void init(boards_config_t* boardsConfig);

	/**
	 * Get copy of a state value.
	 *
	 * @param[in,out] data        Data struct with state type, target, and size.
	 * @param[in] mode            Indicates to get data from RAM, FLASH, FIRMWARE_DEFAULT, or a combination of this.
	 */
	cs_ret_code_t get(cs_state_data_t & data, const PersistenceMode mode = PersistenceMode::STRATEGY1);

	/**
	 * Convenience function for get().
	 *
	 * Avoids having to create a temp every time you want to get a state variable.
	 */
	cs_ret_code_t get(const CS_TYPE type, void *value, const size16_t size);

	/**
	 * Shorthand for get() for boolean data types.
	 */
	bool isTrue(CS_TYPE type, const PersistenceMode mode = PersistenceMode::STRATEGY1);

	/**
	 * Set state to new value, via copy.
	 *
	 * @param[in] data            Data struct with state type, data, and size.
	 * @param[in] mode            Indicates to get data from RAM, FLASH, FIRMWARE_DEFAULT, or a combination of this.
	 */
	cs_ret_code_t set(const cs_state_data_t & data, PersistenceMode mode = PersistenceMode::STRATEGY1);

	/**
	 * Convenience function for set().
	 *
	 * Avoids having to create a temp every time you want to set a state variable.
	 */
	cs_ret_code_t set(const CS_TYPE type, void *value, const size16_t size);

	/**
	 * Verify size of user data for getting a state.
	 *
	 * @param[in] data            Data struct with state type, target, and size.
	 */
	cs_ret_code_t verifySizeForGet(const cs_state_data_t & data);

	/**
	 * Verify size of user data for setting a state.
	 *
	 * @param[in] data            Data struct with state type, data, and size.
	 */
	cs_ret_code_t verifySizeForSet(const cs_state_data_t & data);

	/**
	 * Remove a state variable.
	 */
	cs_ret_code_t remove(CS_TYPE type, const PersistenceMode mode = PersistenceMode::STRATEGY1);

	/**
	 * Erase all used persistent storage.
	 */
	void factoryReset(uint32_t resetCode);

	/**
	 * Get pointer to state value.
	 *
	 * You can use this to avoid copying large state values of fixed length.
	 * However, you have to make sure to call setViaPointer() immediately after any change made in the data.
	 *
	 * @param[in,out] data        Data struct with state type and size.
	 */
	cs_ret_code_t getViaPointer(cs_state_data_t & data, const PersistenceMode mode = PersistenceMode::STRATEGY1);

	/**
	 * Update state value.
	 *
	 * Call this function after data has been changed via pointer. This makes sure the update will be propagated correctly.
	 *
	 * @param[in] type            The state type that has been changed.
	 */
	cs_ret_code_t setViaPointer(CS_TYPE type);

protected:

	Storage* _storage;

	boards_config_t* _boardsConfig;

//	cs_ret_code_t verify(CS_TYPE type, uint8_t* payload, uint8_t length);

//	bool readFlag(CS_TYPE type, bool& value);

	cs_ret_code_t storeInRam(const cs_state_data_t & data);

	cs_ret_code_t loadFromRam(cs_state_data_t & data);

	/**
	 * Stores state variable in ram.
	 *
	 * Allocates memory when not in ram yet, or when size changed.
	 */
	cs_ret_code_t storeInRam(const cs_state_data_t & data, size16_t & index_in_ram);

	cs_ret_code_t allocate(cs_state_data_t & data);

//	std::vector<CS_TYPE> _notifyingStates;

	std::vector<cs_state_data_t> _data_in_ram;

private:

	//! State constructor, singleton, thus made private
	State();

	//! State copy constructor, singleton, thus made private
	State(State const&);

	//! Assignment operator, singleton, thus made private
	void operator=(State const &);
};
