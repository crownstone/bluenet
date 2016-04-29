/*
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 28, 2016
 * License: LGPLv3+
 */
#pragma once

#include <drivers/cs_Serial.h>
#include <structs/buffer/cs_StreamBuffer.h>

#include <ble/cs_Stack.h>
#include <util/cs_Utils.h>

#include <events/cs_EventTypes.h>
#include <events/cs_EventDispatcher.h>

#include <ble/cs_UUID.h>
#include <drivers/cs_Storage.h>

#include <drivers/cs_CyclicStorage.h>

/** Configuration types
 *
 * Use in the characteristic to read and write configurations in <CommonService>.
 */
enum StateVarTypes {
	SV_RESET_COUNTER = StateVar_Base,  // 0x80 - 128
	SV_SWITCH_STATE,                   // 0x81 - 129
	SV_ACCUMULATED_POWER,              // 0x82 - 130
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

#define ACCUMULATED_POWER_REDUNDANCY 50
#define ACCUMULATED_POWER_DEFAULT 0
typedef uint64_t accumulated_power_t;

// State variables ///////////////////////////////

/** Struct used to store elements that are changed frequently. each element
 *  will be stored separately. elements need to be 4 byte sized
 */
struct ps_state_vars_t : ps_storage_base_t {
	// switch state
	uint8_t switchState[SWITCH_STATE_REDUNDANCY * ELEM_SIZE(switch_state_t)];
	// counts resets
	uint8_t resetCounter[RESET_COUNTER_REDUNDANCY * ELEM_SIZE(reset_counter_t)];
	// accumulated power
//	uint8_t accumulatedPower[ACCUMULATED_POWER_REDUNDANCY * ELEM_SIZE(accumulated_power_t)];
};

using namespace BLEpp;

/**
 * Load StateVars from and save StateVars to persistent storage.
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
	void print();
	//	void writeToStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer) {
	void writeToStorage(uint8_t type, uint8_t* payload, uint8_t length, bool persistent = true);

	bool readFromStorage(uint8_t type, StreamBuffer<uint8_t>* streamBuffer);

	ps_state_vars_t& getStateVars();

//	/** Get a handle to the persistent storage struct and load it from FLASH.
//	 *
//	 * Persistent storage is implemented in FLASH. Just as with SSDs, it is important to realize that
//	 * writing less than a minimal block strains the memory just as much as flashing the entire block.
//	 * Hence, there is an entire struct that can be filled and flashed at once.
//	 */
//	void loadPersistentStorage();

//	/** Save to FLASH.
//	 */
//	void savePersistentStorage();

//	void loadFromStorage();
//	void writeStateVars();

//	void ConfigHelper::enable(ps_storage_id id, uint16_t size) {
//		Storage::getInstance().getHandle(id, _storageHandles[id]);
//		loadPersistentStorage(_storageHandles[id], );
//	}

//	/** Retrieve the Bluetooth name from the object representing the BLE stack.
//	 *
//	 * @return name of the device
//	 */
//	std::string & getBLEName();

//	/** Write the Bluetooth name to the object representing the BLE stack.
//	 *
//	 * This updates the Bluetooth name immediately, however, it does not update the name persistently. It
//	 * has to be written to FLASH in that case.
//	 */
//	void setBLEName(const std::string &name, bool persistent = true);

	bool getStateVar(uint8_t type, uint32_t& target);
	bool setStateVar(uint8_t type, uint32_t value);

protected:
	//	pstorage_handle_t _storageHandles[PS_ID_TYPES];
	//	ps_configuration_t* _storageStructs[PS_ID_TYPES];

//	//! handle to storage (required to write to and read from FLASH)
//	pstorage_handle_t _storageHandle;

	pstorage_handle_t _stateVarsHandle;

//	//! struct that storage object understands
//	ps_configuration_t _storageStruct;

	ps_state_vars_t _stateVars;

	CyclicStorage<reset_counter_t, RESET_COUNTER_REDUNDANCY>* _resetCounter;
	CyclicStorage<switch_state_t, SWITCH_STATE_REDUNDANCY>* _switchState;
//	CyclicStorage<accumulated_power_t, ACCUMULATED_POWER_REDUNDANCY>* _accumulatedPower;

	pstorage_size_t getOffset(uint8_t* var);

//	bool returnUint32(uint8_t type, StreamBuffer<uint8_t>* streamBuffer, uint32_t& value);

//	//! non-persistent configuration options
//	std::string _wifiStateVars;

	/*
	 * Writes value from storage to streambuffer
	 */
//	bool getUint16(uint8_t type, StreamBuffer<uint8_t>* streamBuffer, uint32_t value, uint16_t defaultValue);
//	bool getInt8(uint8_t type, StreamBuffer<uint8_t>* streamBuffer, int32_t value, int8_t defaultValue);
//	bool getUint8(uint8_t type, StreamBuffer<uint8_t>* streamBuffer, uint32_t value, uint8_t defaultValue);

//	bool setUint16(uint8_t type, uint8_t* payload, uint8_t length, bool persistent, uint32_t& target);
//	bool setInt8(uint8_t type, uint8_t* payload, uint8_t length, bool persistent, int32_t& target);
//	bool setUint8(uint8_t type, uint8_t* payload, uint8_t length, bool persistent, uint32_t& target);

	bool getStateVar(uint8_t type, StreamBuffer<uint8_t>* streamBuffer, uint32_t value, uint32_t defaultValue);
	bool setStateVar(uint8_t type, uint8_t* payload, uint8_t length, bool persistent, uint32_t& target);
};


