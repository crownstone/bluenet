/**
 * @file cs_Service.h
 * BLE service
 *
 * @authors Crownstone Team, Christopher Mason.
 * @copyright Crownstone B.V.
 * @date Apr 23, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Characteristic.h>
#include <ble/cs_Stack.h>
#include <cfg/cs_Strings.h>
#include <common/cs_BaseClass.h>
#include <common/cs_Tuple.h>
#include <drivers/cs_Timer.h>
#include <util/cs_BleError.h>

class Stack;
class CharacteristicBase;

/** Service as defined in the GATT Specification.
 */
class Service: public BaseClass<1> {
public:
	enum condition_t { C_SERVICE_INITIALIZED };

	//! The "Generic Service"
	static const char* defaultServiceName;

	//! A container with characteristics (underlying data format is a std::vector).
	typedef tuple<CharacteristicBase*> Characteristics_t;

	Service();

	/** Default empty destructor
	 *
	 * We don't currently delete our characteristics as we don't really support dynamic service destruction.
	 * If we wanted to allow services to be removed at runtime, we would need to, amongst many other things,
	 * keep track of whether we allocated the characteristic or whether it was passed into us.
	 */
	virtual ~Service() {
	}

	/** Set the name of the service.
	 *
	 * The name of the service is a const char pointer. Setting the parameters can be done in a chained manner, the
	 * function returns Service again.
	 */
	Service& setName(const char * const name) {
		_name = name;
		return *this;
	}

	/** Set the UUID.
	 *
	 * The UUID cannot be set anymore after the service has been started.
	 */
	Service& setUUID(const UUID& uuid);

	Stack* getStack() {
		return _stack;
	}

	const UUID& getUUID() const {
		return _uuid;
	}

	uint16_t getHandle() {
		return _service_handle;
	}

protected:
	friend class Stack;

	//! Back reference to the stack.
	Stack*  _stack;

	UUID                     _uuid;
	std::string              _name;
	//! Service handle will be obtained from SoftDevice
	uint16_t                 _service_handle;

	//! List of characteristics
	Characteristics_t _characteristics;

	virtual void createCharacteristics() = 0;

	/** Initialization of the service.
	 *
	 * The initialization can be different for each service.
	 */
	virtual void init(Stack* stack);

	virtual void on_ble_event(const ble_evt_t * p_ble_evt);

	virtual void on_connect(uint16_t conn_handle, const ble_gap_evt_connected_t& gap_evt);

	virtual void on_disconnect(uint16_t conn_handle, const ble_gap_evt_disconnected_t& gap_evt);

	virtual bool on_write(const ble_gatts_evt_write_t& write_evt, uint16_t value_handle);

	virtual void onTxComplete(const ble_common_evt_t * p_ble_evt);

	/** Get list of characteristics
	 *
	 * @return list of characteristics
	 */
	virtual Characteristics_t & getCharacteristics() {
		return _characteristics;
	}

	/** Add a single characteristic to the list
	 * @characteristic Characteristic to add
	 */
	virtual Service& addCharacteristic(CharacteristicBase* characteristic) {
		_characteristics.push_back(characteristic);
		return *this;
	}

	Service& updatedCharacteristics() {
		_characteristics.shrink_to_fit();
		return *this;
	}

	void setAesEncrypted(bool encrypted);

};
