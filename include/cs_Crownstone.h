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
#if EDDYSTONE==1
#include <ble/cs_Eddystone.h>
#endif

#include <storage/cs_Settings.h>
#include <events/cs_EventListener.h>

#include <services/cs_IndoorLocalisationService.h>
#include <services/cs_GeneralService.h>
#include <services/cs_PowerService.h>
#include <services/cs_ScheduleService.h>
#include <services/cs_DeviceInformationService.h>
#include <services/cs_SetupService.h>
#include <services/cs_CrownstoneService.h>

#include <processing/cs_CommandHandler.h>
#include <processing/cs_TemperatureGuard.h>
#include <processing/cs_Switch.h>
#include <processing/cs_Scanner.h>
#include <processing/cs_Tracker.h>
#include <processing/cs_PowerSampling.h>
#include <processing/cs_Scheduler.h>
#include <processing/cs_FactoryReset.h>
#include <processing/cs_Watchdog.h>

#if BUILD_MESHING == 1
#include <mesh/cs_Mesh.h>
#include <mesh/cs_MeshControl.h>
#endif
#include <storage/cs_State.h>


/** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** **
 * Main functionality
 ** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

#define CROWNSTONE_UPDATE_FREQUENCY 1 //! hz

/**
 * Crownstone encapsulates all functionality, stack, services, and configuration.
 */
class Crownstone : EventListener {

public:
	Crownstone();

	/** initialize the crownstone:
	 *    1. start UART
	 *    2. configure (drivers, stack, advertisement)
	 *    3. check operation mode
	 *    		3a. for setup mode, create setup services
	 *    		3b. for normal operation mode, create crownstone services
	 *    			and prepare Crownstone for operation
	 *    4. initialize services
	 */
	void init();

	/** startup the crownstone:
	 * 	  1. start advertising
	 * 	  2. start processing (mesh, scanner, tracker, tempGuard, etc)
	 */
	void startUp();

	/** run main
	 *    1. execute app scheduler
	 *    2. wait for ble events
	 */
	void run();

	/** handle (crownstone) events
	 */
	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

	/** tick function called by app timer
	 */
	static void staticTick(Crownstone *ptr) {
		ptr->tick();
	}

protected:

	/** initialize drivers (timer, storage, pwm, etc)
	 */
	void initDrivers();

	/** configure the crownstone. this will call the other configureXXX functions in turn
	 *    1. configure ble stack
	 *    2. set ble name
	 *    3. configure advertisement
	 *
	 */
	void configure();
	void configureStack();
	void configureAdvertisement();
	void setName();

	/** create services available in setup mode
	 */
	void createSetupServices();
	/** create services available in normal operation mode
	 */
	void createCrownstoneServices();

	/** prepare crownstone for normal operation mode, this includes:
	 *    - prepare mesh
	 *    - prepare scanner
	 *    - prepare tracker
	 *    - ...
	 */
	void prepareNormalOperationMode();

	/** tick function for crownstone to update/execute periodically
	 */
	void tick();

	/** schedule next execution of tick function
	 */
	void scheduleNextTick();

private:

	// drivers
	Nrf51822BluetoothStack* _stack;
	Timer* _timer;
	Storage* _storage;
	Settings* _settings;
	State* _stateVars;
	Switch* _switch;
	TemperatureGuard* _temperatureGuard;
	PowerSampling* _powerSampler;

	// services
	DeviceInformationService* _deviceInformationService;
	CrownstoneService* _crownstoneService;
	SetupService* _setupService;
	GeneralService* _generalService;
	IndoorLocalizationService* _localizationService;
	PowerService* _powerService;
	ScheduleService* _scheduleService;

	// advertise
	ServiceData* _serviceData;
	IBeacon* _beacon;
#if EDDYSTONE==1
	Eddystone* _eddystone;
#endif

	// processing
#if BUILD_MESHING == 1
	Mesh* _mesh;
#endif
    CommandHandler* _commandHandler;
	Scanner* _scanner;
	Tracker* _tracker;
	Scheduler* _scheduler;
	FactoryReset* _factoryReset;
	Watchdog* _watchdog;

	bool _advertisementPaused;

#if (NORDIC_SDK_VERSION >= 11)
	app_timer_t              _mainTimerData;
	app_timer_id_t           _mainTimerId;
#else
	uint32_t                 _mainTimerId;
#endif

	uint8_t _operationMode;

};


