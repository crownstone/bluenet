/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jul 13, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include <ble/cs_Service.h>
#include <ble/cs_Characteristic.h>

//#define STRINGIFY(str) #str

class DeviceInformationService : public BLEpp::Service {
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
	void init();

protected:
	// The characteristics in this service, based on:
	// https://developer.bluetooth.org/TechnologyOverview/Pages/DIS.aspx
//	void addManufacturerNameCharacteristic();
//	void addModelNumberCharacteristic();
//	void addSerialNumberCharacteristic();
	void addHardwareRevisionCharacteristic();
	void addFirmwareRevisionCharacteristic();
//	void addSoftwareRevisionCharacteristic();
//	void addSystemIDCharacteristic();
//	void addRegulatoryCertificationDataListCharacteristic();

private:
	// References to characteristics that need to be written from other functions
//	BLEpp::Characteristic<std::string> *_manufacturerNameCharacteristic;
//	BLEpp::Characteristic<std::string> *_modelNumberCharacteristic;
//	BLEpp::Characteristic<std::string> *_serialNumberCharacteristic;
	BLEpp::Characteristic<std::string> *_hardwareRevisionCharacteristic;
	BLEpp::Characteristic<std::string> *_firmwareRevisionCharacteristic;
//	BLEpp::Characteristic<std::string> *_softwareRevisionCharacteristic;
//	BLEpp::Characteristic<std::string> *_systemIdCharacteristic;
//	BLEpp::Characteristic<std::string> *_regulatoryCerificationDataListCharacteristic;
};
