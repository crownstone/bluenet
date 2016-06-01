/*
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Apr 24, 2015
 * License: LGPLv3+
 */
#pragma once

/** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** **
 * General includes
 ** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

#include <ble/cs_Stack.h>
#include <ble/cs_iBeacon.h>

#include <cfg/cs_Settings.h>
#include <cfg/cs_StateVars.h>

#include <events/cs_EventListener.h>

#include <services/cs_IndoorLocalisationService.h>
#include <services/cs_GeneralService.h>
#include <services/cs_PowerService.h>
#include <services/cs_AlertService.h>
#include <services/cs_ScheduleService.h>
#include <services/cs_DeviceInformationService.h>

#include <processing/cs_CommandHandler.h>
#include <processing/cs_TemperatureGuard.h>
#include <processing/cs_Sensors.h>
#include <processing/cs_Fridge.h>
#include <processing/cs_Switch.h>
#include <processing/cs_Scanner.h>
#include <processing/cs_Tracker.h>

#include <protocol/cs_Mesh.h>


/** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** **
 * Main functionality
 ** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

#define CROWNSTONE_UPDATE_FREQUENCY 1 //! hz

using namespace BLEpp;

/**
 * Crownstone encapsulates all functionality, stack, services, and configuration.
 */
class Crownstone : EventListener {

public:
	Crownstone();

	void init();

	void setup();

	void startUp();

	void run();

	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

	static void staticTick(Crownstone *ptr) {
		ptr->tick();
	}

protected:
	void welcome();

	void setName();
	void configStack();
	void configAdvertisement();
	void configDrivers();

	void createSetupServices();
	void createCrownstoneServices();

	void configure();

//	void startAdvertising();

	void tick();
	void scheduleNextTick();

private:

	// drivers
	Nrf51822BluetoothStack* _stack;
	Timer* _timer;
	Storage* _storage;
	Settings* _settings;
	StateVars* _stateVars;
	Switch* _switch;
	TemperatureGuard* _temperatureGuard;

	// services
	GeneralService* _generalService;
	IndoorLocalizationService* _localizationService;
	PowerService* _powerService;
	AlertService* _alertService;
	ScheduleService* _scheduleService;
	DeviceInformationService* _deviceInformationService;

	// advertise
	ServiceData* _serviceData;
	IBeacon* _beacon;

	// processing
	CMesh* _mesh;
	Sensors* _sensors;
	Fridge* _fridge;
	CommandHandler* _commandHandler;
	Scanner* _scanner;
	Tracker* _tracker;

	bool _advertisementPaused;

	app_timer_id_t _mainTimer;

	uint32_t _operationMode;

};


