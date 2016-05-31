/*
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 28, 2016
 * License: LGPLv3+
 */
#pragma once

#include <drivers/cs_CyclicStorage.h>
#include <events/cs_EventTypes.h>
#include <structs/buffer/cs_StreamBuffer.h>

#define OPERATION_MODE_SETUP 0xFF
#define OPERATION_MODE_NORMAL 0x10
#define OPERATION_MODE_DFU 0x01

//! enable to print additional debug
//#define PRINT_DEBUG

/** State variable types
 *
 * Use in the characteristic to read and write state variables in <CommonService>.
 */
enum StateVarTypes {
	SV_RESET_COUNTER = StateVar_Base,  // 0x80 - 128
	SV_SWITCH_STATE,                   // 0x81 - 129
	SV_ACCUMULATED_POWER,              // 0x82 - 130
	SV_POWER_CONSUMPTION,              // 0x83 - 131
	SV_TRACKED_DEVICES,                // 0x84 - 132
	SV_SCHEDULE,                       // 0x85 - 133
	SV_OPERATION_MODE,                 // 0x86 - 134
	SV_TEMPERATURE,                    // 0x87 - 135
	STATE_VAR_TYPES
};

typedef uint32_t seq_number_t;

#define ELEM_SIZE(e) (sizeof(e) + sizeof(seq_number_t))

#define RESET_COUNTER_REDUNDANCY 10
#define RESET_COUNTER_DEFAULT -1
typedef uint32_t reset_counter_t;

#define SWITCH_STATE_REDUNDANCY 10
#define SWITCH_STATE_DEFAULT 0
typedef uint32_t switch_state_t;

#define ACCUMULATED_POWER_REDUNDANCY 72
#define ACCUMULATED_POWER_DEFAULT 0
typedef uint64_t accumulated_power_t;

/** Struct used to store elements that are changed frequently. each element
 *  will be stored separately. elements need to be 4 byte sized
 */
struct ps_state_vars_t : ps_storage_base_t {
	// switch state
	uint8_t switchState[SWITCH_STATE_REDUNDANCY * ELEM_SIZE(switch_state_t)];
	// counts resets
	uint8_t resetCounter[RESET_COUNTER_REDUNDANCY * ELEM_SIZE(reset_counter_t)];
	// accumulated power
	uint8_t accumulatedPower[ACCUMULATED_POWER_REDUNDANCY * ELEM_SIZE(accumulated_power_t)];
}; // FULL

//! size of one block in eeprom can't be bigger than 1024 bytes. => create a new struct
STATIC_ASSERT(sizeof(ps_state_vars_t) <= 1024);

//todo: lists need to be adjusted to use cyclic storages for wear leveling
/** Struct used to ...
 */
struct __attribute__ ((aligned (4))) ps_general_vars_t : ps_storage_base_t {
	uint32_t operationMode;
	uint8_t trackedDevices[sizeof(tracked_device_list_t)];
	uint8_t scheduleList[sizeof(schedule_list_t)];
};

//! size of one block in eeprom can't be bigger than 1024 bytes. => create a new struct
STATIC_ASSERT(sizeof(ps_general_vars_t) <= 1024);

struct state_vars_notifaction {
	uint8_t type;
	uint8_t* data;
	uint16_t dataLength;
};

/**
 * Load StateVars from and save StateVars to persistent storage.
 *
 * State variables are variables that are frequently updated by the application
 * and represent different states of the application. state variables are generally
 * stored with a cyclic storage for wear leveling. this way the default number of
 * write cycles of the eeprom can be extended and the eeprom is used equally.
 */
class StateVars {

private:
	StateVars();

	//! This class is singleton, deny implementation
	StateVars(StateVars const&);
	//! This class is singleton, deny implementation
	void operator=(StateVars const &);

public:
	static StateVars& getInstance() {
		static StateVars instance;
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
	void writeToStorage(uint8_t type, uint8_t* payload, uint8_t length, bool persistent = true);

	/** Read a state variable from storage and return as a stream buffer (to be read in characteristic)
	 */
	bool readFromStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer);

	/** Get a state variable from storage (to be used in application)
	 */
	bool getStateVar(uint8_t type, uint32_t& target);
	bool getStateVar(uint8_t type, uint8_t& target);
	bool getStateVar(uint8_t type, int32_t& target);
	bool getStateVar(uint8_t type, buffer_ptr_t buffer, uint16_t size);

	/** Write a state variable to storage
	 */
	bool setStateVar(uint8_t type, uint8_t value);
	bool setStateVar(uint8_t type, uint32_t value);
	bool setStateVar(uint8_t type, int32_t value);
	bool setStateVar(uint8_t type, buffer_ptr_t buffer, uint16_t size);

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

protected:

	void publishUpdate(uint8_t type, uint8_t* data, uint16_t size);

	bool _initialized;

	Storage* _storage;

	//! handle to the storage
	pstorage_handle_t _stateVarsHandle;

	pstorage_handle_t _structHandle;
	ps_general_vars_t _storageStruct;

	//! counts application resets (only for debug purposes?)
	CyclicStorage<reset_counter_t, RESET_COUNTER_REDUNDANCY>* _resetCounter;
	//! keeps track of the switch state, i.e. current PWM value
	CyclicStorage<switch_state_t, SWITCH_STATE_REDUNDANCY>* _switchState;
	//! keeps track of the accumulated power
	CyclicStorage<accumulated_power_t, ACCUMULATED_POWER_REDUNDANCY>* _accumulatedPower;

	std::vector<uint8_t> _notifyingStateVars;

	//! state variables which do not need to be stored in storage:
	int32_t _temperature;

};


