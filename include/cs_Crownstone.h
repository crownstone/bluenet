/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Apr 24, 2015
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

/** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** **
 * General includes
 ** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

#include <ble/cs_Stack.h>
#include <ble/cs_iBeacon.h>
#include <cfg/cs_Boards.h>
#include <events/cs_EventListener.h>
#include <processing/cs_CommandHandler.h>
#include <processing/cs_FactoryReset.h>
#include <processing/cs_PowerSampling.h>
#include <processing/cs_Scanner.h>
#include <processing/cs_Scheduler.h>
#include <processing/cs_Switch.h>
#include <processing/cs_TemperatureGuard.h>
#include <processing/cs_Watchdog.h>
#include <services/cs_CrownstoneService.h>
#include <services/cs_DeviceInformationService.h>
#include <services/cs_SetupService.h>
#include <storage/cs_State.h>
#include <storage/cs_State.h>
#if BUILD_MESHING == 1
#include <mesh/cs_Mesh.h>
#include <mesh/cs_MeshControl.h>
#endif

/** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** **
 * Main functionality
 ** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

#define CROWNSTONE_UPDATE_FREQUENCY 2 //! hz

/**
 * Crownstone encapsulates all functionality, stack, services, and configuration.
 */
class Crownstone : EventListener {
public:

    enum ServiceEvent { 
	CREATE_DEVICE_INFO_SERVICE, CREATE_SETUP_SERVICE, CREATE_CROWNSTONE_SERVICE, 
	REMOVE_DEVICE_INFO_SERVICE, REMOVE_SETUP_SERVICE, REMOVE_CROWNSTONE_SERVICE };

    /**
     * Constructor.
     */
    Crownstone(boards_config_t& board);

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
    void handleEvent(event_t & event);

    /** tick function called by app timer
    */
    static void staticTick(Crownstone *ptr) {
	ptr->tick();
    }

protected:

    /** initialize drivers (stack, timer, storage, pwm, etc), loads settings from storage.
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

    void writeDefaults();

    void createService(const ServiceEvent event);
    
    /** Create services available in setup mode
     */
    void createSetupServices();

    /** Create services available in normal operation mode
     */
    void createCrownstoneServices();

    /** Start operation for Crownstone. 
     * 
     * This currently only enables parts in Normal operation mode. This includes:
     *    - prepare mesh
     *    - prepare scanner
     *    - prepare tracker
     *    - ...
     */
    void startOperationMode(const OperationMode & mode);
    
    void switchMode(const OperationMode & mode);

    /** tick function for crownstone to update/execute periodically
     */
    void tick();

    /** schedule next execution of tick function
     */
    void scheduleNextTick();

    /** Increase reset counter. This will be stored in FLASH so it persists over reboots.
     */
    void increaseResetCounter();
private:

    boards_config_t _boardsConfig;

    // drivers
    Stack* _stack;
    Timer* _timer;
    Storage* _storage;
    State* _state;

    Switch* _switch;
    TemperatureGuard* _temperatureGuard;
    PowerSampling* _powerSampler;
    Watchdog* _watchdog;

    // services
    DeviceInformationService* _deviceInformationService;
    CrownstoneService* _crownstoneService;
    SetupService* _setupService;

    // advertise
    ServiceData* _serviceData;
    IBeacon* _beacon;

    // processing
#if BUILD_MESHING == 1
    Mesh* _mesh;
#endif
    CommandHandler* _commandHandler;
    Scanner* _scanner;
    Scheduler* _scheduler;
    FactoryReset* _factoryReset;

    app_timer_t              _mainTimerData;
    app_timer_id_t           _mainTimerId;

    OperationMode _operationMode;

};


