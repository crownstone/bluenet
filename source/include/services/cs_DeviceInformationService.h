/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jul 13, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Characteristic.h>
#include <ble/cs_Service.h>

/** The DeviceInformationService is a BLE service that gives info on hardware and firmware revisions.
 */
class DeviceInformationService : public Service {
public:
	/** Constructor for alert notification service object
	 *
	 * Creates persistent storage (FLASH) object which is used internally to store current limit.
	 * It also initializes all characteristics.
	 */
	DeviceInformationService();

	/** Initialize a GeneralService object
	 *
	 * Add all characteristics and initialize them where necessary.
	 */
	void createCharacteristics();

protected:
	void addHardwareRevisionCharacteristic();
	void addFirmwareRevisionCharacteristic();
	void addSoftwareRevisionCharacteristic();

private:
	Characteristic<const char*>* _hardwareRevisionCharacteristic = nullptr;
	Characteristic<const char*>* _firmwareRevisionCharacteristic = nullptr;
	Characteristic<const char*>* _softwareRevisionCharacteristic = nullptr;
};
