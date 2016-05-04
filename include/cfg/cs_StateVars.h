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
};

//! size of one block in eeprom can't be bigger than 1024 bytes. => create a new struct
STATIC_ASSERT(sizeof(ps_state_vars_t) <= 1024);

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

	/** Write a state variable to storage
	 */
	bool setStateVar(uint8_t type, uint32_t value);

protected:

	//! handle to the storage
	pstorage_handle_t _stateVarsHandle;

	//! counts application resets (only for debug purposes?)
	CyclicStorage<reset_counter_t, RESET_COUNTER_REDUNDANCY>* _resetCounter;
	//! keeps track of the switch state, i.e. current PWM value
	CyclicStorage<switch_state_t, SWITCH_STATE_REDUNDANCY>* _switchState;
	//! keeps track of the accumulated power
	CyclicStorage<accumulated_power_t, ACCUMULATED_POWER_REDUNDANCY>* _accumulatedPower;

};


