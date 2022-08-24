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

/**
 * Base class for a BLE service
 */
class Service : public BaseClass<1> {
public:
	enum condition_t { C_SERVICE_INITIALIZED };

	Service();

	/**
	 * Default empty destructor
	 *
	 * We don't currently delete our characteristics as we don't really support dynamic service destruction.
	 * If we wanted to allow services to be removed at runtime, we would need to, amongst many other things,
	 * keep track of whether we allocated the characteristic or whether it was passed into us.
	 */
	virtual ~Service() {}

	/**
	 * Set the UUID.
	 *
	 * The UUID cannot be set anymore after the service has been initialized.
	 */
	void setUUID(const UUID& uuid);

	Stack* getStack() { return _stack; }

	const UUID& getUUID() const { return _uuid; }

	uint16_t getHandle() { return _handle; }

protected:
	friend class Stack;

	//! Back reference to the stack.
	Stack* _stack = nullptr;

	UUID _uuid;

	//! Service handle will be obtained from SoftDevice
	uint16_t _handle = BLE_CONN_HANDLE_INVALID;

	//! List of characteristics
	tuple<CharacteristicBase*> _characteristics;

	virtual void createCharacteristics() = 0;

	/**
	 * Initialize the service: register it at the softdevice.
	 */
	void init(Stack* stack);

	void onBleEvent(const ble_evt_t* event);

	void onConnect(uint16_t connectionHandle, const ble_gap_evt_connected_t& event);

	void onDisconnect(uint16_t connectionHandle, const ble_gap_evt_disconnected_t& event);

	bool onWrite(const ble_gatts_evt_write_t& event, uint16_t gattHandle);

	void onTxComplete(const ble_common_evt_t* event);

	/** Add a single characteristic to the list
	 * @characteristic Characteristic to add
	 */
	void addCharacteristic(CharacteristicBase* characteristic) { _characteristics.push_back(characteristic); }

	void updatedCharacteristics() { _characteristics.shrink_to_fit(); }

	void setAesEncrypted(bool encrypted);
};
