/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Characteristic.h>
#include <ble/cs_Nordic.h>
#include <ble/cs_Service.h>
#include <events/cs_EventListener.h>
#include <services/cs_CrownstoneService.h>

//#define GENERAL_SERVICE_UPDATE_FREQUENCY 10 //! hz

/** Setup Service for the Crownstone
 *
 */
class SetupService : public CrownstoneService {
public:
	/** Setup Service derivces from Crownstone Service.
	 */
	SetupService();

	/** Initialize the Setup Service object.
	 *
	 * Add all characteristics and initialize them if necessary. The function will not call createCharacteristics
	 * of the super class CrownstoneService.
	 */
	void createCharacteristics();

	void handleEvent(event_t& event);

protected:
	inline void addMacAddressCharacteristic();

	inline void addSetupKeyCharacteristic(buffer_ptr_t buffer, uint16_t size);

private:
	// stores the MAC address of the devices to be used for mesh message handling
	ble_gap_addr_t _myAddr;

	uint8_t _keyBuffer[SOC_ECB_KEY_LENGTH];

	Characteristic<buffer_ptr_t>* _macAddressCharacteristic;
	Characteristic<buffer_ptr_t>* _setupKeyCharacteristic;
};
