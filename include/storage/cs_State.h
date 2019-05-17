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
#include <drivers/cs_Timer.h>

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
 * Struct for a type of which storing to flash is queued or delayed.
 *
 * The type will be stored once the counter is 0.
 */
struct __attribute__((__packed__)) cs_state_store_queue_t {
	CS_TYPE type;
	uint16_t counter;
};

/**
 * Stores state values in RAM and/or FLASH.
 *
 * Each state type always has a FIRMWARE_DEFAULT value.
 * Values that are set, always take precedence over the FIRMWARE_DEFAULT.
 *
 * Values like chip temperature, that don't have to be stored over reboots should be stored in and read from RAM.
 *
 * Values like CONFIG_BOOT_DELAY should be known over reboots of the device. Moreover, they should also persist
 * over firmware updates. These values are stored in FLASH.
 * If the values are not changed, they should NOT be stored to FLASH. They can then immediately be read from the
 * FIRMWARE_DEFAULT. If these values are stored in FLASH, they always take precedence over FIRMWARE_DEFAULT values.
 * Values that are stored in FLASH will be cached in RAM, to prevent having to read from FLASH every time.
 * This complete persistence strategy is called STRATEGY1.
 *
 * If a new firmware has a new FIRMWARE_DEFAULT that should be enforced, we need to explicitly remove the value from
 * FLASH before updating the firmware.
 * For example, a new CONFIG_BOOT_DELAY may have to be enforced, or else it might go into an infinite reboot loop.
 *
 * For each state type it is defined whether it should be stored in RAM or FLASH.
 *
 * All values that are stored to flash should also be stored in RAM. This is required, because storing to flash is
 * asynchronous, the value must remain in RAM at least until the write is done.
 *
 * Storing to flash can also fail, in which case it can be retried later. The state type will be put in a queue and
 * retried after about STATE_RETRY_STORE_DELAY_MS ms.
 *
 *
 * Get procedure:
 *   1. Read RAM
 *     - if present, return RAM value.
 *   2. Get location (FLASH or RAM)
 *   3a. If RAM
 *     - get default value.
 *     - store default value in RAM.
 *   3b. If FLASH
 *     - read FLASH
 *     - if present, copy FLASH value to RAM.
 *     - if not present, get default value, store default value in RAM.
 *   4. Copy RAM value to return value.
 *
 * Set procedure:
 *   1. Copy value to RAM.
 *   2. Get location (FLASH or RAM)
 *     - If RAM, return.
 *   3. Write to storage.
 *     - If success, return.
 *     - If busy, add state type to queue, return.
 */
class State: public BaseClass<>, EventListener {
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
	 * Set the state to a new value, but delay the write to flash.
	 *
	 * Assumes persistence mode STRATEGY1.
	 * Use this when you know a lot of sets of the same type may be done in a short time period.
	 * Each time this function is called, the timeout is reset.
	 *
	 * @param[in] data            Data struct with state type, data, and size.
	 * @param[in] delay           How long the delay should be, in seconds.
	 */
	cs_ret_code_t setDelayed(const cs_state_data_t & data, uint8_t delay);

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
	 *
	 * TODO: implement.
	 */
	cs_ret_code_t remove(CS_TYPE type, const PersistenceMode mode = PersistenceMode::STRATEGY1);

	/**
	 * Erase all used persistent storage.
	 */
	void factoryReset(uint32_t resetCode);

	/**
	 * Get pointer to state value.
	 *
	 * TODO: implement
	 * You can use this to avoid copying large state values of fixed length.
	 * However, you have to make sure to call setViaPointer() immediately after any change made in the data.
	 *
	 * @param[in,out] data        Data struct with state type and size.
	 */
	cs_ret_code_t getViaPointer(cs_state_data_t & data, const PersistenceMode mode = PersistenceMode::STRATEGY1);

	/**
	 * Update state value.
	 *
	 * TODO: implement
	 * Call this function after data has been changed via pointer. This makes sure the update will be propagated correctly.
	 *
	 * @param[in] type            The state type that has been changed.
	 */
	cs_ret_code_t setViaPointer(CS_TYPE type);

	/**
	 * After calling this, state is allowed to write to flash. Should be called once all reads are finished.
	 *
	 * This is a quickfix to prevent failing flash reads during garbage collection.
	 * To properly fix this, read should be blocking. This can be done by handling FDS events in the interrupt, instead
	 * of via the app_scheduler. In those events, the busy state can be cleared, while cs events can be dispatched via
	 * the app_scheduler. Then, the read function can just wait with nrf_delay() until no longer busy.
	 */
	void startWritesToFlash();

	/**
	 * Handle (crownstone) events.
	 */
	void handleEvent(event_t & event);

protected:

	Storage* _storage;

	boards_config_t* _boardsConfig;

//	cs_ret_code_t verify(CS_TYPE type, uint8_t* payload, uint8_t length);

//	bool readFlag(CS_TYPE type, bool& value);

	/**
	 * Find given type in ram.
	 *
	 * @param[in]                 Type to search for.
	 * @param[out]                Index in ram register where the type was found.
	 * @return                    ERR_SUCCESS when type was found.
	 * @return                    ERR_NOT_FOUND when type was not found.
	 */
	cs_ret_code_t findInRam(const CS_TYPE & type, size16_t & index_in_ram);

	cs_ret_code_t storeInRam(const cs_state_data_t & data);

	cs_ret_code_t loadFromRam(cs_state_data_t & data);

	/**
	 * Stores state variable in ram.
	 *
	 * Allocates memory when not in ram yet, or when size changed.
	 */
	cs_ret_code_t storeInRam(const cs_state_data_t & data, size16_t & index_in_ram);

	/**
	 * Writes state variable in ram to flash.
	 */
	cs_ret_code_t storeInFlash(size16_t & index_in_ram);

	/**
	 * Adds a new state_data struct to ram.
	 *
	 * Allocates the struct and the data pointer.
	 *
	 * @param[in] type            State type.
	 * @param[in] size            State variable size.
	 * @return                    Struct with allocated data pointer.
	 */
	cs_state_data_t & addToRam(const CS_TYPE & type, size16_t size);

	cs_ret_code_t addToQueue(const CS_TYPE & type, uint32_t delayMs);

	cs_ret_code_t allocate(cs_state_data_t & data);

	void delayedStoreTick();

//	std::vector<CS_TYPE> _notifyingStates;

	std::vector<cs_state_data_t> _ram_data_register;

	std::vector<cs_state_store_queue_t> _store_queue;

	bool _startedWritingToFlash = false;

private:

	//! State constructor, singleton, thus made private
	State();

	//! State copy constructor, singleton, thus made private
	State(State const&);

	//! Assignment operator, singleton, thus made private
	void operator=(State const &);
};
