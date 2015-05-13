/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 24, 2015
 * License: LGPLv3+
 */
#pragma once

/**********************************************************************************************************************
 * General includes
 *********************************************************************************************************************/

#include <ble/cs_Stack.h>

#include <ble/cs_iBeacon.h>

#include <services/cs_IndoorLocalisationService.h>
#include <services/cs_GeneralService.h>
#include <services/cs_PowerService.h>

#include <processing/cs_Sensors.h>

#include <events/cs_EventListener.h>

/**********************************************************************************************************************
 * Main functionality
 *********************************************************************************************************************/

using namespace BLEpp;

class Crownstone : EventListener {

private:
	Nrf51822BluetoothStack* _stack;

	GeneralService* _generalService;
	IndoorLocalizationService* _localizationService;
	PowerService* _powerService;

	IBeacon* _beacon;
	Sensors* _sensors;

	void welcome();

	void setName();
	void configStack();
	void configDrivers();

	void createServices();

public:
	Crownstone() : _stack(NULL),
		_generalService(NULL), _localizationService(NULL), _powerService(NULL),
		_beacon(NULL), _sensors(NULL) {};

	void setup();

	void run();

	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

};


