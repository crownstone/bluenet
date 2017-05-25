/*
 * Author: Dominik Egger
 * Copyright: Crownstone B.V. (https://crownstone.rocks)
 * Date: Apr 28, 2016
 * License: LGPLv3+, Apache, MIT
 */
#pragma once

#include <events/cs_EventTypes.h>
#include <structs/cs_StreamBuffer.h>
#include <storage/cs_CyclicStorage.h>
#include <protocol/cs_StateTypes.h>
#include <protocol/cs_ErrorCodes.h>
#include <processing/cs_EnOceanHandler.h>
#include <vector>

#define SWITCH_STATE_PERSISTENT

#define OPERATION_MODE_SETUP                       0x00
#define OPERATION_MODE_DFU                         0x01
#define OPERATION_MODE_FACTORY_RESET               0x02
#define OPERATION_MODE_NORMAL                      0x10

typedef uint16_t small_seq_number_t;
typedef uint32_t large_seq_number_t;

#define ELEM_SIZE(e, c) (sizeof(e) + sizeof(c))

#define RESET_COUNTER_REDUNDANCY 20
#define RESET_COUNTER_DEFAULT -1
typedef uint16_t reset_counter_t;

#define SWITCH_STATE_REDUNDANCY 200
typedef uint16_t switch_state_storage_t;

#define ACCUMULATED_ENERGY_REDUNDANCY 200
#define ACCUMULATED_ENERGY_DEFAULT 0
typedef int32_t accumulated_energy_t;

/** 
 * The ps_state_t struct is used to store elements that are changed relatively frequently. Each element is stored 
 * separately (the struct is not persisted in its entirety because that would lead to unnecessary writes to FLASH). 
 * The writes to FLASH are on the level of words (4 bytes). This is a requirement enforced by pstorage and not
 * conforming to it leads to runtime errors.
 *
 * The nRF52 has 512 kB FLASH at 0x00000000 to 0x00080000 in the memory map. With the bootloader at the end of the FLASH,
 * the last page available for pstorage is from address 0x78000 to 0x79FFF and is used as internal pstorage swap page.
 * If there are two pages reserved for the application data, it is two pages below the pstorage swap page in the
 * memory map (addresses 76000 to 77FFF).
 *
 * The nRF52 has 10.000 erase/write cycles. The X_REDUNDANCY macros define redundancy in writing to FLASH. Each write
 * a counter is shifted with the size of the field to be stored. This counter is stored with the field and is persisted
 * as well. If not, we would not be able to  pick the right persisted value.
 *
 * A macro such as SWITCH_STATE_REDUNCACY equal to 200 means 2.000.000 erase/write cycles. Assuming a lifetime of 10 years,
 * this amounts to 87.600 hours. Hence, this redundancy allows us to toggle state 20 times per hour and persist that state.
 * 
 * The ACCUMULATED_ENERGY_REDUNDANCY at 200 is 2.000.000 erase/write cycles. There are 5.256.000 minutes in 10 years,
 * hence this amounts to every 2 and a half minute.
 *
 * TODO: The seq_number_t counter struct is 16-bits, while 8-bits would already be sufficient.
 * TODO: The switch_state_storage_t field is 16-bits, while 8-bits would be sufficient (1 on/off bit, 7 dimming bits).
 */
struct ps_state_t : ps_storage_base_t {
	// counts resets
	uint8_t resetCounter[RESET_COUNTER_REDUNDANCY * ELEM_SIZE(reset_counter_t, small_seq_number_t)];
	// switch state
	uint8_t switchState[SWITCH_STATE_REDUNDANCY * ELEM_SIZE(switch_state_storage_t, small_seq_number_t)];
	// accumulated power
	uint8_t accumulatedEnergy[ACCUMULATED_ENERGY_REDUNDANCY * ELEM_SIZE(accumulated_energy_t, large_seq_number_t)];
};

//! size of one block in eeprom can't be bigger than 0x1000 bytes. => create a new struct
STATIC_ASSERT(sizeof(ps_state_t) <= 0x1000);

//todo: lists need to be adjusted to use cyclic storages for wear leveling
/** Struct used to ...
 */
struct __attribute__ ((aligned (4))) ps_general_vars_t : ps_storage_base_t {
	uint32_t operationMode;
	uint8_t trackedDevices[sizeof(tracked_device_list_t)] __attribute__ ((aligned (4)));
	uint8_t scheduleList[sizeof(schedule_list_t)] __attribute__ ((aligned (4)));
	uint8_t learnedSwitches[MAX_SWITCHES * sizeof(learned_enocean_t)] __attribute__ ((aligned (4)));
};

//! size of one block in eeprom can't be bigger than 0x1000 bytes. => create a new struct
STATIC_ASSERT(sizeof(ps_general_vars_t) <= 0x1000);

struct state_vars_notifaction {
	uint8_t type;
	uint8_t* data;
	uint16_t dataLength;
};

union state_errors_t {
	struct __attribute__((packed)) {
		uint8_t overCurrent : 1;
		uint8_t overCurrentPwm : 1;
		uint8_t chipTemp : 1;
		uint8_t pwmTemp : 1;
		uint32_t reserved : 28;
	} errors;
	uint32_t asInt;
};

#define FACTORY_RESET_STATE_NORMAL 0
#define FACTORY_RESET_STATE_LOWTX  1
#define FACTORY_RESET_STATE_RESET  2

/**
 * Load StateVars from and save StateVars to persistent storage.
 *
 * State variables are variables that are frequently updated by the application
 * and represent different states of the application. state variables are generally
 * stored with a cyclic storage for wear leveling. this way the default number of
 * write cycles of the eeprom can be extended and the eeprom is used equally.
 */
class State {

private:
	State();

	//! This class is singleton, deny implementation
	State(State const&);
	//! This class is singleton, deny implementation
	void operator=(State const &);

public:
	static State& getInstance() {
		static State instance;
		return instance;
	}

	void init();

	bool isInitialized() {
		return _initialized;
	}

	/** Print debug information
	 */
	void print();

	/** Write a state variable to storage (if received via stream buffer from characteristic)
	 */
	ERR_CODE writeToStorage(uint8_t type, uint8_t* payload, uint8_t length, bool persistent = true);

	/** Read a state variable from storage and return as a stream buffer (to be read in characteristic)
	 */
	ERR_CODE readFromStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer);

	/** Get a state variable from storage (to be used in application)
	 */
	ERR_CODE get(uint8_t type, uint32_t& target) {
		return get(type, &target, sizeof(uint32_t));
	}
	ERR_CODE get(uint8_t type, uint16_t& target) {
		return get(type, &target, sizeof(uint16_t));
	}
	ERR_CODE get(uint8_t type, uint8_t& target) {
		return get(type, &target, sizeof(uint8_t));
	}
	ERR_CODE get(uint8_t type, int32_t& target) {
		return get(type, &target, sizeof(int32_t));
	}
//	ERR_CODE get(uint8_t type, buffer_ptr_t buffer, uint16_t size);

	/** Write a state variable to storage
	 */
	ERR_CODE set(uint8_t type, uint8_t value) {
		return set(type, &value, sizeof(uint8_t));
	}
	ERR_CODE set(uint8_t type, uint16_t value) {
		return set(type, &value, sizeof(uint16_t));
	}
	ERR_CODE set(uint8_t type, uint32_t value) {
		return set(type, &value, sizeof(uint32_t));
	}
	ERR_CODE set(uint8_t type, int32_t value) {
		return set(type, &value, sizeof(value));
	}
//	ERR_CODE set(uint8_t type, buffer_ptr_t buffer, uint16_t size);

	ERR_CODE verify(uint8_t type, uint16_t size);
	ERR_CODE set(uint8_t type, void* target, uint16_t size);
	ERR_CODE get(uint8_t type, void* target, uint16_t size);

	uint16_t getStateItemSize(uint8_t type);

	/** Factory Resets sets all variables to their default values and clears FLASH storage
	 */
	void factoryReset(uint32_t resetCode);

	/** Get a handle to the persistent storage struct and load it from FLASH into working memory.
	 *
	 * Persistent storage is implemented in FLASH. Just as with SSDs, it is important to realize that
	 * writing less than a minimal block strains the memory just as much as flashing the entire block.
	 * Hence, there is an entire struct that can be filled and flashed at once.
	 */
	void loadPersistentStorage();

	/** Save the whole struct (working memory) to FLASH.
	 */
	void savePersistentStorage();

	void savePersistentStorageItem(uint8_t* item, uint16_t size);

	void setNotify(uint8_t type, bool enable);
	bool isNotifying(uint8_t type);
	void disableNotifications();

protected:

	void publishUpdate(uint8_t type, uint8_t* data, uint16_t size);

	bool _initialized;

	Storage* _storage;

	//! handle to the storage
	pstorage_handle_t _stateHandle;

	pstorage_handle_t _structHandle;
	ps_general_vars_t _storageStruct;

	//! counts application resets (only for debug purposes?)
	CyclicStorage<reset_counter_t, RESET_COUNTER_REDUNDANCY, small_seq_number_t>* _resetCounter;
	//! keeps track of the switch state, i.e. current PWM value
#ifdef SWITCH_STATE_PERSISTENT
	CyclicStorage<switch_state_storage_t, SWITCH_STATE_REDUNDANCY, small_seq_number_t>* _switchState;
#else
	uint8_t _switchState;
#endif
	//! keeps track of the accumulated power
	CyclicStorage<accumulated_energy_t, ACCUMULATED_ENERGY_REDUNDANCY, large_seq_number_t>* _accumulatedEnergy;

	std::vector<uint8_t> _notifyingStates;

	//! state variables which do not need to be stored in persistent storage:
	int32_t _temperature;
	int32_t _powerUsage;
	uint32_t _time;
	uint8_t _factoryResetState;
	state_errors_t _errorState;

};


