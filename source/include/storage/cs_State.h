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
#include <drivers/cs_Timer.h>
#include <events/cs_EventListener.h>
#include <protocol/cs_ErrorCodes.h>
#include <vector>

constexpr const char* operationModeName(OperationMode const & mode) {
    switch(mode) {
		case OperationMode::OPERATION_MODE_SETUP:
			return "SETUP";
		case OperationMode::OPERATION_MODE_DFU:
			return "DFU";
		case OperationMode::OPERATION_MODE_FACTORY_RESET:
			return "FACTORY_RESET";
		case OperationMode::OPERATION_MODE_NORMAL:
			return "NORMAL";
		case OperationMode::OPERATION_MODE_UNINITIALIZED:
			return "UNINITIALIZED";
    }
    // should never be reached
    return "Unknown mode!";
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

enum class StateQueueMode {
	DELAY,
	THROTTLE
};

#define FACTORY_RESET_STATE_NORMAL 0
#define FACTORY_RESET_STATE_LOWTX  1
#define FACTORY_RESET_STATE_RESET  2

enum StateQueueOp {
	CS_STATE_QUEUE_OP_WRITE,
	CS_STATE_QUEUE_OP_REM_ONE_ID_OF_TYPE,
	CS_STATE_QUEUE_OP_FACTORY_RESET,
	CS_STATE_QUEUE_OP_GC,
};

/**
 * Struct for queuing operations.
 *
 * operation:      Type of operation to perform.
 * type:           State type
 * id:             State id
 * counter:        Number of ticks until item is executed and removed from queue.
 * init_counter:   When set, and execute is true, this item is added again with counter set to this value.
 * execute:        Whether or not to execute the operation when the counter reaches 0.
 */
struct __attribute__((__packed__)) cs_state_store_queue_t {
	StateQueueOp operation;
	CS_TYPE type;
	cs_state_id_t id;
	uint32_t counter; // Uint32, so it can fit 24h.
	uint32_t init_counter;
	bool execute;
};

const uint32_t CS_STATE_QUEUE_DELAY_SECONDS_MAX = 0xFFFFFFFF / 1000;

struct cs_id_list_t {
	CS_TYPE type;
	std::vector<cs_state_id_t>* ids;
	cs_id_list_t(CS_TYPE type, std::vector<cs_state_id_t>* ids):
		type(type),
		ids(ids)
	{}
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
	/**
	 * Get a reference to the State object.
	 */
	static State& getInstance() {
		static State instance;
		return instance;
	}

	~State();

	/**
	 * Initialize the State object with the board configuration.
	 *
	 * @param[in] boardsConfig    Board configuration (pin layout, etc.).
	 */
	void init(boards_config_t* boardsConfig);

	/**
	 * Get copy of a state value.
	 *
	 * @param[in,out] data        Data struct with state type, id, data, and size. See cs_state_data_t.
	 * @param[in] mode            Indicates whether to get data from RAM, FLASH, FIRMWARE_DEFAULT, or a combination of this.
	 * @return                    Return code.
	 */
	cs_ret_code_t get(cs_state_data_t & data, const PersistenceMode mode = PersistenceMode::STRATEGY1);

	/**
	 * Convenience function for get() with id 0.
	 *
	 * Avoids having to create a temp every time you want to get a state variable.
	 */
	cs_ret_code_t get(const CS_TYPE type, void *value, const size16_t size);

	/**
	 * Shorthand for get() for boolean data types, and id 0.
	 *
	 * @param[in] type            State type.
	 * @param[in] mode            Indicates whether to get data from RAM, FLASH, FIRMWARE_DEFAULT, or a combination of this.
	 * @return                    True when state type is true.
	 */
	bool isTrue(CS_TYPE type, const PersistenceMode mode = PersistenceMode::STRATEGY1);

	/**
	 * Get a list of IDs for given type.
	 *
	 * Example usage:
	 *     std::vector<cs_state_id_t>* ids = nullptr;
	 *     retCode = State::getInstance().getIds(CS_TYPE::STATE_EXAMPLE, ids);
	 *     if (retCode == ERR_SUCCESS) {
	 *         for (auto id: *ids) {
	 *
	 * @param[in] type            State type.
	 * @param[out] ids            Pointer to list of ids. This is not a copy, so make sure not to modify this list.
	 * @return                    Return code.
	 */
	cs_ret_code_t getIds(CS_TYPE type, std::vector<cs_state_id_t>* & ids);

	/**
	 * Set state to new value, via copy.
	 *
	 * @param[in] data            Data struct with state type, optional id, data, and size.
	 * @param[in] mode            Indicates whether to set data in RAM, FLASH, or a combination of this.
	 * @return                    Return code. Can be ERR_SUCCESS_NO_CHANGE.
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
	 * @return                    Return code.
	 */
	cs_ret_code_t setDelayed(const cs_state_data_t & data, uint8_t delay);
	
	/**
	 * Write variable to flash in a throttled mode.
	 *
	 * This function setThrottled will immediately write a value to flash, except when this has happened in the last
	 * period (indicated by a parameter). If this function is called during this period, the value will be updated in
	 * ram. Only after this period this value will then be written to flash. Also after this time, this will lead to a
	 * period in which throttling happens. For example, when the period parameter is set to 60000, all calls to
	 * setThrottled will result to calls to flash at a maximum rate of once per minute (for given data type).
	 *
	 * @param[in] data           Data to store.
	 * @param[in] period         Period in seconds that throttling will be in effect. Must be smaller than CS_STATE_QUEUE_DELAY_SECONDS_MAX.
	 * @return                   Return code (e.g. ERR_SUCCES, ERR_WRONG_PARAMETER).
	 */
	cs_ret_code_t setThrottled(const cs_state_data_t & data, uint32_t period);

	/**
	 * Verify size of user data for getting a state.
	 *
	 * @param[in] data            Data struct with state type, target, and size.
	 * @return                    Return code.
	 */
	cs_ret_code_t verifySizeForGet(const cs_state_data_t & data);

	/**
	 * Verify size of user data for setting a state.
	 *
	 * @param[in] data            Data struct with state type, data, and size.
	 * @return                    Return code.
	 */
	cs_ret_code_t verifySizeForSet(const cs_state_data_t & data);

	/**
	 * Remove a state variable.
	 *
	 * @param[in] type            The state type to be removed.
	 * @param[in] mode            Indicates whether to remove data from RAM, FLASH, or a combination of this.
	 * @return                    Return code.
	 */
	cs_ret_code_t remove(const CS_TYPE & type, cs_state_id_t id, const PersistenceMode mode = PersistenceMode::STRATEGY1);

	/**
	 * Erase all used persistent storage.
	 */
	void factoryReset();

	/**
	 * Clean up persistent storage.
	 *
	 * Permanently deletes removed state variables, and defragments the persistent storage.
	 *
	 * @return                    Success when cleaning up will be started.
	 */
	cs_ret_code_t cleanUp();

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
	 * Internal usage
	 */
	void handleStorageError(cs_storage_operation_t operation, CS_TYPE type, cs_state_id_t id);

	/**
	 * Handle (crownstone) events.
	 */
	void handleEvent(event_t & event);

protected:

	Storage* _storage;

	boards_config_t* _boardsConfig;

	/**
	 * Find given type in ram.
	 *
	 * @param[in]                 Type to search for.
	 * @param[in]                 Id to search for.
	 * @param[out]                Index in ram register where the type was found.
	 * @return                    ERR_SUCCESS when type was found.
	 * @return                    ERR_NOT_FOUND when type was not found.
	 */
	cs_ret_code_t findInRam(const CS_TYPE & type, cs_state_id_t id, size16_t & index_in_ram);

	/**
	 * Stores state variable in ram.
	 *
	 * Allocates memory when not in ram yet, or when size changed.
	 *
	 * @param[in] data            Data to be stored.
	 * @param[out] index_in_ram   Index where the data is stored.
	 * @return                    Return code.
	 */
	cs_ret_code_t storeInRam(const cs_state_data_t & data, size16_t & index_in_ram);

	/**
	 * Convenience function, in case you're not interested in index in ram.
	 */
	cs_ret_code_t storeInRam(const cs_state_data_t & data);

	/**
	 * Copies from ram to target buffer.
	 *
	 * Does not check if target buffer has a large enough size.
	 *
	 * @param[in]  data.type      Type of data.
	 * @param[in]  data.id        Identifier of the data (to get a particular instance of a type).
	 * @param[out] data.size      The size of the data retrieved.
	 * @param[out] data.value     The data itself.
	 *
	 * @return                    Return value (i.e. ERR_SUCCESS or other).
	 */
	cs_ret_code_t loadFromRam(cs_state_data_t & data);

	/**
	 * Adds a new state_data struct to ram.
	 *
	 * Allocates the struct and the data pointer.
	 *
	 * @param[in] type            State type.
	 * @param[in] size            State variable size.
	 * @return                    Struct with allocated data pointer.
	 */
	cs_state_data_t & addToRam(const CS_TYPE & type, cs_state_id_t id, size16_t size);

	/**
	 * Removed a state variable from ram.
	 *
	 * Frees the data pointer and struct.
	 *
	 * @param[in] type            State type.
	 * @return                    Return code.
	 */
	cs_ret_code_t removeFromRam(const CS_TYPE & type, cs_state_id_t id);

	/**
	 * Check a particular value with the value currently in ram.
	 *
	 * @param[in]  data           Data with type, id, value, and size.
	 * @param[out] cmp_result     Value that indicates comparison (equality indicated by 0).
	 * @return                    Return code (e.g. ERR_SUCCESS, ERR_NOT_FOUND, etc.)
	 */
	cs_ret_code_t compareWithRam(const cs_state_data_t & data, uint32_t & cmp_result);

	/**
	 * Writes state variable in ram to flash.
	 *
	 * Can return ERR_BUSY, in which case you have to retry again later.
	 *
	 * @param[in] index           Index in ram with the data to be written.
	 * @return                    Return code.
	 */
	cs_ret_code_t storeInFlash(size16_t & index_in_ram);

	/**
	 * Remove given id of given type from flash.
	 *
	 * Can return ERR_BUSY, in which case you have to retry again later.
	 *
	 * @param[in] type            State type.
	 * @param[in] id              State value id.
	 * @return                    Return code.
	 */
	cs_ret_code_t removeFromFlash(const CS_TYPE & type, const cs_state_id_t id);

	/**
	 * Add an operation to queue.
	 *
	 * Entries with the same type and operation will be overwritten.
	 * Write and remove of the same type will also overwrite each other.
	 *
	 * @param[in] operation       Type of operation.
	 * @param[in] fileId          Flash file ID of the entry.
	 * @param[in] type            State type.
	 * @param[in] delayMs         Delay in ms.
	 * @return                    Return code.
	 */
	cs_ret_code_t addToQueue(StateQueueOp operation, const CS_TYPE & type, cs_state_id_t id, uint32_t delayMs,
			const StateQueueMode mode);

	cs_ret_code_t allocate(cs_state_data_t & data);

	void delayedStoreTick();

	/**
	 * Stores state data structs with pointers to state data.
	 */
	std::vector<cs_state_data_t> _ram_data_register;

	/**
	 * Stores list of existing ids for certain types.
	 */
	std::vector<cs_id_list_t> _idsCache;

	/**
	 * Stores the queue of flash operations.
	 */
	std::vector<cs_state_store_queue_t> _store_queue;

	bool _startedWritingToFlash = false;

	bool _performingFactoryReset = false;

private:

	//! State constructor, singleton, thus made private
	State();

	//! State copy constructor, singleton, thus made private
	State(State const&);

	//! Assignment operator, singleton, thus made private
	void operator=(State const &);

	cs_ret_code_t setInternal(const cs_state_data_t & data, PersistenceMode mode);

	cs_ret_code_t removeInternal(const CS_TYPE & type, cs_state_id_t id, const PersistenceMode mode);

	/**
	 * Handle factory reset result.
	 *
	 * @retrun                    False when factory reset needs to be retried.
	 */
	bool handleFactoryResetResult(cs_ret_code_t retCode);

	cs_ret_code_t getDefaultValue(cs_state_data_t & data);

	/**
	 * Get and cache all IDs with given type from flash.
	 *
	 * @param[in] type            State type.
	 * @param[out] ids            List of ids.
	 * @return                    Return code.
	 */
	cs_ret_code_t getIdsFromFlash(const CS_TYPE & type, std::vector<cs_state_id_t>* & ids);

	/**
	 * Add ID to list of cached IDs.
	 *
	 * Will _NOT_ create a new list if no list for this type exists.
	 */
	cs_ret_code_t addId(const CS_TYPE & type, cs_state_id_t id);

	/**
	 * Remove ID from list of cached IDs.
	 */
	cs_ret_code_t remId(const CS_TYPE & type, cs_state_id_t id);
};
