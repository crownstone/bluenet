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


#include <behaviour/cs_BehaviourStore.h>
#include <ble/cs_Advertiser.h>
#include <ble/cs_BleCentral.h>
#include <ble/cs_CrownstoneCentral.h>
#include <ble/cs_iBeacon.h>
#include <ble/cs_Stack.h>
#include <cfg/cs_Boards.h>
#include <events/cs_EventListener.h>
#include <localisation/cs_AssetFiltering.h>
#include <localisation/cs_MeshTopology.h>
#include <presence/cs_PresenceHandler.h>
#include <processing/cs_CommandAdvHandler.h>
#include <processing/cs_CommandHandler.h>
#include <processing/cs_FactoryReset.h>
#include <processing/cs_MultiSwitchHandler.h>
#include <processing/cs_PowerSampling.h>
#include <processing/cs_Scanner.h>
#include <processing/cs_TemperatureGuard.h>
#include <services/cs_CrownstoneService.h>
#include <services/cs_DeviceInformationService.h>
#include <services/cs_SetupService.h>
#include <storage/cs_State.h>
#include <switch/cs_SwitchAggregator.h>
#include <time/cs_SystemTime.h>
#include <tracking/cs_TrackedDevices.h>

#if BUILD_MESHING == 1
#include <mesh/cs_Mesh.h>
#endif

#if BUILD_MICROAPP_SUPPORT == 1
#include <microapp/cs_Microapp.h>
#endif

#if BUILD_MEM_USAGE_TEST == 1
#include <test/cs_MemUsageTest.h>
#endif

#if BUILD_TWI == 1
#include <drivers/cs_Twi.h>
#endif

#if BUILD_GPIOTE == 1
#include <drivers/cs_Gpio.h>
#endif

/** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** **
 * Main functionality
 ** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** */

/**
 * Crownstone encapsulates all functionality, stack, services, and configuration.
 */
class Crownstone : EventListener {
public:

	enum ServiceEvent {
		CREATE_DEVICE_INFO_SERVICE, CREATE_SETUP_SERVICE, CREATE_CROWNSTONE_SERVICE,
		REMOVE_DEVICE_INFO_SERVICE, REMOVE_SETUP_SERVICE, REMOVE_CROWNSTONE_SERVICE };

	/** Allocate Crownstone class and internal references.
	 *
	 * Create buffers, timers, storage, state, etc. We are running on an embedded device. Only allocate something in a
	 * constructor. For dynamic information use the stack. Do not allocate/deallocate anything during runtime. It is
	 * too risky. It might not be freed and memory might overflow. This type of hardware should run months without
	 * interruption.
	 *
	 * There is a function IS_CROWNSTONE. What this actually is contrasted with are other BLE type devices, in particular
	 * the Guidestone. The latter devices do not have the ability to switch, dim, or measure power consumption.
	 *
	 * The order with which items are created.
	 *
	 *  + buffers
	 *  + event distpatcher
	 *  + BLE stack
	 *  + timer
	 *  + persistent storage
	 *  + state
	 *  + command handler
	 *  + factory reset
	 *  + scanner
	 *  + tracker
	 *  + mesh
	 *  + switch
	 *  + temperature guard
	 *  + power sampling
	 *
	 * The initialization is done in separate function.
	 */
	Crownstone(boards_config_t& board);

	/**
	 * Initialize the crownstone:
	 *    1. start UART
	 *    2. configure (drivers, stack, advertisement)
	 *    3. check operation mode
	 *    		3a. for setup mode, create setup services
	 *    		3b. for normal operation mode, create crownstone services
	 *    			and prepare Crownstone for operation
	 *    4. initialize services
	 *
	 * Since some classes initialize asynchronous, this function is called multiple times to continue the process.
	 * When the class is initialized, the init done event will continue the initialization of other classes.
	 */
	/**
	 * Initialize Crownstone firmware. First drivers are initialized (log modules, storage modules, ADC conversion,
	 * timers). Then everything is configured independent of the mode (everything that is common to whatever mode the
	 * Crownstone runs on). A callback to the local staticTick function for a timer is set up. Then the mode of
	 * operation is switched and the BLE services are initialized.
	 */
	void init(uint16_t step);

	/** startup the crownstone:
	 * 	  1. start advertising
	 * 	  2. start processing (mesh, scanner, tracker, tempGuard, etc)
	 */
	/**
	 * After allocation of all modules, after initialization of each module, and after configuration of each module, we
	 * are ready to "start". This means:
	 *
	 *   - advertise
	 *   - turn on/off switch at boot (depending on default)
	 *   - watch temperature excess
	 *   - power sampling
	 *   - schedule tasks
	 */
	void startUp();

	/** run main
	 *    1. execute app scheduler
	 *    2. wait for ble events
	 */
	/**
	 * An infinite loop in which the application ceases control to the SoftDevice at regular times. It runs the scheduler,
	 * waits for events, and handles them. Also the printed statements in the log module are flushed.
	 */
	void run();

	/** handle (crownstone) events
	 */
	/**
	 * Handle events that can come from other parts of the Crownstone firmware and even originate from outside of the
	 * firmware via the BLE interface (an application, or the mesh).
	 */
	void handleEvent(event_t & event);

	/**
	 * Update the heap statistics.
	 */
	static void updateHeapStats();

	/**
	 * Update the minimal stack end location.
	 *
	 * Should be called regularly from high level interrupt.
	 */
	static void updateMinStackEnd();

	/**
	 * Print load stats: RAM usage, app scheduler usage, etc.
	 */
	static void printLoadStats();

	/** tick function called by app timer
	 */
	static void staticTick(Crownstone *ptr) {
		ptr->tick();
	}

private:

	/**
	 * The distinct init phases.
	 */
	void init0();
	void init1();

	/** initialize drivers (stack, timer, storage, pwm, etc), loads settings from storage.
	 */
	/**
	 * This must be called after the SoftDevice has started. The order in which things should be initialized is as follows:
	 *   1. Stack.               Starts up the softdevice. It controls a lot of devices, so need to set it early.
	 *   2. Timer.               Also initializes the app scheduler.
	 *   3. Storage.             Definitely after the stack has been initialized.
	 *   4. State.               Storage should be initialized here.
	 */
	void initDrivers(uint16_t step);

	/**
	 * The distinct driver init phases.
	 */
	void initDrivers0();
	void initDrivers1();

	/** configure the crownstone. this will call the other configureXXX functions in turn
	 *    1. configure ble stack
	 *    2. set ble name
	 *    3. configure advertisement
	 *
	 */
	/**
	 * Configure the Bluetooth stack. This also increments the reset counter.
	 *
	 * The order within this function is important. For example setName() sets the BLE device name and
	 * configureAdvertisements() defines advertisement parameters on appearance. These have to be called after
	 * the storage has been initialized.
	 */
	void configure();

	/** Sets default parameters of the Bluetooth connection.
	 *
	 * Data is transmitted with TX_POWER dBm.
	 *
	 * On transmission of data within a connection (higher interval -> lower power consumption, slow communication)
	 *   - minimum connection interval (in steps of 1.25 ms, 16*1.25 = 20 ms)
	 *   - maximum connection interval (in steps of 1.25 ms, 32*1.25 = 40 ms)
	 * The supervision timeout multiplier is 400
	 * The slave latency is 10
	 * On advertising:
	 *   - advertising interval (in steps of 0.625 ms, 1600*0.625 = 1 sec) (can be between 0x0020 and 0x4000)
	 *   - advertising timeout (disabled, can be between 0x0001 and 0x3FFF, and is in steps of seconds)
	 *
	 * There is no whitelist defined, nor peer addresses.
	 *
	 * Process:
	 *   [31.05.16] we used to stop / start scanning after a disconnect, now starting advertising is enough
	 *   [23.06.16] restart the mesh on disconnect, otherwise we have ~10s delay until the device starts advertising.
	 *   [29.06.16] restart the mesh disabled, this was limited to pca10000, it does crash dobeacon v0.7
	 */

	/**
	 * Populate advertisement (including service data) with information. The persistence mode is obtained from storage
	 * (the _operationMode var is not used).
	 */
	void configureAdvertisement();

	/**
	 * The default name. This can later be altered by the user if the corresponding service and characteristic is enabled.
	 * It is loaded from memory or from the default and written to the Stack.
	 */
	void setName(bool firstTime);

	/**
	 * Create a particular service. Depending on the mode we can choose to create a set of services that we would need.
	 * After creation of a service it cannot be deleted. This is a restriction of the Nordic Softdevice. If you need a
	 * new set of services, you will need to change the mode of operation (in a persisted field). Then you can restart
	 * the device and use the mode to enable another set of services.
	 */
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
	/**
	 * Start the different modules depending on the operational mode. For example, in normal mode we use a scanner and
	 * the mesh. In setup mode we use the serial module (but only RX).
	 */
	void startOperationMode(const OperationMode & mode);

	/** Switch from one operation mode to another.
	 *
	 * Depending on the operation mode we have a different set of services / characteristics enabled. Subsequently,
	 * also different entities are started (for example a scanner, or the BLE mesh).
	 */
	void switchMode(const OperationMode & mode);

	/** tick function for crownstone to update/execute periodically
	 */
	/**
	 * Operations that are not sensitive with respect to time and only need to be called at regular intervals.
	 * TODO: describe function calls and why they are required.
	 */
	void tick();

	/** schedule next execution of tick function
	 */
	void scheduleNextTick();

	/** Increase reset counter. This will be stored in FLASH so it persists over reboots.
	 */
	/**
	 * Increase the reset counter. This can be used for debugging purposes. The reset counter is written to FLASH and
	 * persists over reboots.
	 */
	void increaseResetCounter();

	boards_config_t _boardsConfig;

	// drivers
	Stack* _stack;
	BleCentral* _bleCentral;
	CrownstoneCentral* _crownstoneCentral;
	Advertiser* _advertiser;
	Timer* _timer;
	Storage* _storage;
	State* _state;

	TemperatureGuard* _temperatureGuard = nullptr;
	PowerSampling* _powerSampler = nullptr;

	// services
	DeviceInformationService* _deviceInformationService = nullptr;
	CrownstoneService* _crownstoneService = nullptr;
	SetupService* _setupService = nullptr;

	// advertise
	ServiceData* _serviceData = nullptr;

	// processing
#if BUILD_MESHING == 1
	Mesh* _mesh = nullptr;
#endif
	CommandHandler* _commandHandler = nullptr;
	Scanner* _scanner = nullptr;
	FactoryReset* _factoryReset = nullptr;
	CommandAdvHandler* _commandAdvHandler = nullptr;
	MultiSwitchHandler* _multiSwitchHandler = nullptr;
	TrackedDevices _trackedDevices;
	SystemTime _systemTime;

	MeshTopology _meshTopology;

	SwitchAggregator _switchAggregator;
	AssetFiltering _assetFiltering;

	BehaviourStore _behaviourStore;
	PresenceHandler _presenceHandler;

#if BUILD_MICROAPP_SUPPORT == 1
	Microapp* _microapp;
#endif

#if BUILD_MEM_USAGE_TEST == 1
	MemUsageTest _memTest;
#endif

#if BUILD_TWI == 1
	Twi* _twi = nullptr;
#endif

#if BUILD_GPIOTE == 1
	Gpio* _gpio = nullptr;
#endif

	app_timer_t              _mainTimerData;
	app_timer_id_t           _mainTimerId;
	TYPIFY(EVT_TICK) _tickCount = 0;

	OperationMode _operationMode;
	OperationMode _oldOperationMode = OperationMode::OPERATION_MODE_UNINITIALIZED;

	//! Store gpregret as it was on boot.
	uint32_t _gpregret[2] = {0};

	//! Store reset reason as it was on boot.
	uint32_t _resetReason = 0;

	static cs_ram_stats_t _ramStats;

	/**
	 * If storage was recovered by erasing all pages, we want to set some state variables
	 * different than after a factory reset.
	 */
	bool _setStateValuesAfterStorageRecover = false;

	bool _clearedGpRegRetCount = false;
};
