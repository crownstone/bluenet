/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Oct 22, 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include <ble/cs_Nordic.h>

#include <ble/cs_Service.h>
#include <ble/cs_Characteristic.h>
#include <events/cs_EventListener.h>
//#if BUILD_MESHING == 1
//#include <structs/cs_MeshCommand.h>
//#endif
#include <services/cs_CrownstoneService.h>

//#define GENERAL_SERVICE_UPDATE_FREQUENCY 10 //! hz

/** General Service for the Crownstone
 *
 * There are several characteristics that fit into the general service description. There is a characteristic
 * that measures the temperature, there are several characteristics that defines the crownstone, namely by
 * name, by type, or by location (room), and there is a characteristic to update its firmware.
 *
 * If meshing is enabled, it is also possible to send a message into the mesh network using a characteristic.
 */
class SetupService: public CrownstoneService {
public:
	/** Constructor for general crownstone service object
	 *
	 * Creates persistent storage (FLASH) object which is used internally to store name and other information that is
	 * set over so-called configuration characteristics. It also initializes all characteristics.
	 */
	SetupService();
	~SetupService();

	/** Initialize a GeneralService object
	 *
	 * Add all characteristics and initialize them where necessary.
	 */
	void createCharacteristics();

	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

protected:

	inline void addMacAddressCharacteristic();

	inline void addSetupKeyCharacteristic(buffer_ptr_t buffer, uint16_t size);

private:

	// stores the MAC address of the devices to be used for mesh message handling
    ble_gap_addr_t _myAddr;
    uint8_t* _keyBuffer;
	uint8_t* _nonceBuffer;


	Characteristic<buffer_ptr_t>* _macAddressCharacteristic;
	Characteristic<buffer_ptr_t>* _setupKeyCharacteristic;




};
