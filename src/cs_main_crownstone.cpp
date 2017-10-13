/**
 * Author: Crownstone Team
 * Copyright: Crownstone
 * Date: 14 Aug., 2014
 * License: LGPLv3+, Apache, or MIT, your choice
 */

/**********************************************************************************************************************
 *
 * The Crownstone is a high-voltage (domestic) switch. It can be used for:
 *   - indoor localization
 *   - building automation
 *
 * It is one of the first, or the first(?), open-source Internet-of-Things devices entering the market.
 *
 * Read more on: https://crownstone.rocks
 *
 * Almost all configuration options should be set in CMakeBuild.config.
 *
 *********************************************************************************************************************/

#include <cs_Crownstone.h>
#include <ble/cs_CrownstoneManufacturer.h>
#include <cfg/cs_Boards.h>
#include <cfg/cs_HardwareVersions.h>
#include <drivers/cs_PWM.h>
#include <drivers/cs_RNG.h>
#include <drivers/cs_RTC.h>
#include <drivers/cs_Temperature.h>
#include <drivers/cs_Timer.h>
#include <events/cs_EventDispatcher.h>
#include <events/cs_EventTypes.h>
#include <processing/cs_EncryptionHandler.h>
#include <storage/cs_State.h>
#include <structs/buffer/cs_MasterBuffer.h>
#include <structs/buffer/cs_EncryptionBuffer.h>
#include <util/cs_Utils.h>

extern "C" {
#include <nrf_nvmc.h>
}

// Define test pin to enable gpio debug.
//#define TEST_PIN 20

// Define to enable leds. WARNING: this is stored in UICR and not easily reversible!
//#define ENABLE_LEDS

/**********************************************************************************************************************
 * Main functionality
 *********************************************************************************************************************/

void handleZeroCrossing() {
	PWM::getInstance().onZeroCrossing();
}

Crownstone::Crownstone(boards_config_t& board) :
	_boardsConfig(board),
	_switch(NULL), _temperatureGuard(NULL), _powerSampler(NULL), _watchdog(NULL), _enOceanHandler(NULL),
	_deviceInformationService(NULL), _crownstoneService(NULL), _setupService(NULL),
	_generalService(NULL), _localizationService(NULL), _powerService(NULL),
	_scheduleService(NULL),
	_serviceData(NULL), _beacon(NULL),
#if BUILD_MESHING == 1
	_mesh(NULL),
#endif
	_commandHandler(NULL), _scanner(NULL), _tracker(NULL), _scheduler(NULL), _factoryReset(NULL),
	_advertisementPaused(false),
	_mainTimerId(NULL),
	_operationMode(0)
{
	_mainTimerData = { {0} };
	_mainTimerId = &_mainTimerData;

	MasterBuffer::getInstance().alloc(MASTER_BUFFER_SIZE);
	EncryptionBuffer::getInstance().alloc(BLE_GATTS_VAR_ATTR_LEN_MAX);

	EventDispatcher::getInstance().addListener(this);

	//! set up the bluetooth stack that controls the hardware.
	_stack = &Nrf51822BluetoothStack::getInstance();

	// create all the objects that are needed for execution, but make sure they
	// don't execute any softdevice related code. do that in an init function
	// and call it in the setup/configure phase
	_timer = &Timer::getInstance();

	// persistent storage, configuration, state
	_storage = &Storage::getInstance();
	_settings = &Settings::getInstance();
	_stateVars = &State::getInstance();

	//! create command handler
	_commandHandler = &CommandHandler::getInstance();
	_factoryReset = &FactoryReset::getInstance();

	//! create instances for the scanner and mesh
	//! actual initialization is done in their respective init methods
	_scanner = &Scanner::getInstance();
#if CHAR_TRACK_DEVICES == 1
	_tracker = &Tracker::getInstance();
#endif

#if BUILD_MESHING == 1
	_mesh = &Mesh::getInstance();
#endif

	if (IS_CROWNSTONE(_boardsConfig.deviceType)) {
		// switch using PWM or Relay
		_switch = &Switch::getInstance();
		//! create temperature guard
		_temperatureGuard = &TemperatureGuard::getInstance();

		_powerSampler = &PowerSampling::getInstance();

		_watchdog = &Watchdog::getInstance();

		_enOceanHandler = &EnOceanHandler::getInstance();
	}

};

void Crownstone::init() {
	LOGi(FMT_HEADER, "init");

	//! initialize drivers
	initDrivers();
	BLEutil::print_heap("Heap initDrivers: ");
	BLEutil::print_stack("Stack initDrivers: ");


	LOGi(FMT_HEADER, "configure");

	//! configure the crownstone
	configure();
	BLEutil::print_heap("Heap configure: ");
	BLEutil::print_stack("Stack configure: ");

	LOGi(FMT_INIT, "encryption handler");
	BLEutil::print_heap("Heap encryption: ");
	BLEutil::print_stack("Stack encryption: ");

	LOGi(FMT_HEADER, "setup");

	_stateVars->get(STATE_OPERATION_MODE, _operationMode);

	switch(_operationMode) {
	case OPERATION_MODE_SETUP: {

		LOGd("Configure setup mode");

		//! create services
		createSetupServices();

		//! loop through all services added to the stack and create the characteristics
		_stack->createCharacteristics();

		//! set it by default into low tx mode
		_stack->changeToLowTxPowerMode();

		//! Because iPhones cannot programmatically clear their cache of paired devices, the phone that
		//! did the setup is at risk of not being able to connect to the crownstone if the cs clears the device
		//! manager. We use our own encryption scheme to counteract this.
		if (_settings->isSet(CONFIG_ENCRYPTION_ENABLED)) {
			LOGi(FMT_ENABLE, "AES encryption");
			_stack->setAesEncrypted(true);
		}
//		LOGi(FMT_ENABLE, "PIN encryption");
//		//! use PIN encryption for setup mode
//		_stack->setPinEncrypted(true);

		if (_boardsConfig.deviceType == DEVICE_CROWNSTONE_BUILTIN) {
			_switch->delayedSwitch(SWITCH_ON, SWITCH_ON_AT_SETUP_BOOT_DELAY);
		}

		break;
	}
	case OPERATION_MODE_NORMAL: {

		LOGd("Configure normal operation mode");

		//! setup normal operation mode
		prepareNormalOperationMode();

		//! create services
		createCrownstoneServices();

		//! loop through all services added to the stack and create the characteristics
		_stack->createCharacteristics();

		//! use aes encryption for normal mode (if enabled)
		if (_settings->isSet(CONFIG_ENCRYPTION_ENABLED)) {
			LOGi(FMT_ENABLE, "AES encryption");
			_stack->setAesEncrypted(true);
		}

		break;
	}
	case OPERATION_MODE_FACTORY_RESET: {

		LOGd("Configure factory reset mode");

		FactoryReset::getInstance().finishFactoryReset();
		break;
	}
	case OPERATION_MODE_DFU: {

		CommandHandler::getInstance().handleCommand(CMD_GOTO_DFU);
		break;
	}
	}

	LOGi(FMT_HEADER, "init services");

	_stack->initServices();

	// [16.06.16] need to execute app scheduler, otherwise pstorage
	// events will get lost ... maybe need to check why that actually happens??
	app_sched_execute();
}

void Crownstone::configure() {

	LOGi("> stack ...");
	//! configure parameters for the Bluetooth stack
	configureStack();

	uint16_t resetCounter;
	State::getInstance().get(STATE_RESET_COUNTER, resetCounter);
	++resetCounter;
	LOGw("Reset counter at: %d", resetCounter);
	State::getInstance().set(STATE_RESET_COUNTER, resetCounter);

	//! set advertising parameters such as the device name and appearance.
	//! Note: has to be called after _stack->init or Storage is initialized too early and won't work correctly
	setName();

	LOGi("> advertisement ...");
	//! configure advertising parameters
#if EDDYSTONE==1
	_eddystone = new Eddystone();
	_eddystone->advertising_init();
#else
	configureAdvertisement();
#endif
}

/**
 * This must be called after the SoftDevice has started.
 */
void Crownstone::initDrivers() {

#ifdef ENABLE_LEDS
	if (NRF_UICR->NFCPINS != 0) {
		LOGw("enable gpio LEDs");
		nrf_nvmc_write_word(0x1000120C, 0);
	}
#endif
	LOGd("NFC pins: %p", NRF_UICR->NFCPINS);

	LOGi(FMT_INIT, "stack");

	//! start up the softdevice early because we need it's functions to configure devices it ultimately controls.
	//! in particular we need it to set interrupt priorities.
	_stack->init();

	LOGi(FMT_INIT, "timers");
	_timer->init();

	LOGi(FMT_INIT, "pstorage");
	// initialization of storage and settings has to be done **AFTER** stack is initialized
	_storage->init();

	LOGi("Loading configuration");
	_settings->init(&_boardsConfig);
	_stateVars->init();

	LOGi(FMT_INIT, "command handler");
	_commandHandler->init(&_boardsConfig);

	LOGi(FMT_INIT, "factory reset");
	_factoryReset->init();

	LOGi(FMT_INIT, "encryption handler");
	EncryptionHandler::getInstance().init();

	if (IS_CROWNSTONE(_boardsConfig.deviceType)) {
		// switch / PWM init
		LOGi(FMT_INIT, "switch / PWM");
		_switch->init(_boardsConfig);

		LOGi(FMT_INIT, "temperature guard");
		_temperatureGuard->init(_boardsConfig);

		LOGi(FMT_INIT, "power sampler");
		_powerSampler->init(_boardsConfig);

		LOGi(FMT_INIT, "watchdog");
		_watchdog->init();

		_enOceanHandler->init();
	}

	// init GPIOs
	if (_boardsConfig.flags.hasLed) {
		LOGi("Configure LEDs");
		// Note: DO NOT USE THEM WHILE SCANNING OR MESHING ...
		nrf_gpio_cfg_output(_boardsConfig.pinLedRed);
		nrf_gpio_cfg_output(_boardsConfig.pinLedGreen);
		if (_boardsConfig.flags.ledInverted) {
			// setting the pin makes them turn off ....
			nrf_gpio_pin_set(_boardsConfig.pinLedRed);
			nrf_gpio_pin_set(_boardsConfig.pinLedGreen);
		} else {
			nrf_gpio_pin_clear(_boardsConfig.pinLedRed);
			nrf_gpio_pin_clear(_boardsConfig.pinLedGreen);
		}
	}
}

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
void Crownstone::configureStack() {

	_stack->setTxPowerLevel(TX_POWER);
	_stack->setMinConnectionInterval(MIN_CONNECTION_INTERVAL);
	_stack->setMaxConnectionInterval(MAX_CONNECTION_INTERVAL);
	_stack->setConnectionSupervisionTimeout(CONNECTION_SUPERVISION_TIMEOUT);
	_stack->setSlaveLatency(SLAVE_LATENCY);
	_stack->setAdvertisingInterval(ADVERTISEMENT_INTERVAL);
	_stack->setAdvertisingTimeoutSeconds(ADVERTISING_TIMEOUT);

	// Set the stored tx power
	int8_t txPower;
	_settings->get(CONFIG_TX_POWER, &txPower);
	LOGi("Set TX power to %i", txPower);
	_stack->setTxPowerLevel(txPower);

	// Set the stored advertisement interval
	uint16_t advInterval;
	_settings->get(CONFIG_ADV_INTERVAL, &advInterval);
	_stack->setAdvertisingInterval(advInterval);

	// Retrieve and Set the PASSKEY for pairing
	uint8_t passkey[BLE_GAP_PASSKEY_LEN];
	_settings->get(CONFIG_PASSKEY, passkey);

	// TODO: If key is not set, this leads to a crash
	_stack->setPasskey(passkey);

	_stack->onConnect([&](uint16_t conn_handle) {
		LOGi("onConnect...");
		// TODO: see https://devzone.nordicsemi.com/index.php/about-rssi-of-ble
		// be neater about it... we do not need to stop, only after a disconnect we do...
#if RSSI_ENABLE==1
		sd_ble_gap_rssi_stop(conn_handle);
		sd_ble_gap_rssi_start(conn_handle, 0, 0);
#endif
		sd_power_gpregret_clr(0xFF);

		_stack->setNonConnectable();
		_stack->restartAdvertising();

	});

	_stack->onDisconnect([&](uint16_t conn_handle) {
		LOGi("onDisconnect...");
		if (_operationMode == OPERATION_MODE_SETUP) {
			_stack->changeToLowTxPowerMode();
		}

		_stateVars->disableNotifications();

		_stack->setConnectable();
		_stack->restartAdvertising();
	});

}

void Crownstone::configureAdvertisement() {

	//! create the iBeacon parameter object which will be used
	//! to configure advertisement as an iBeacon

	//! get values from config
	uint16_t major, minor;
	uint8_t rssi;
	ble_uuid128_t uuid;

	_settings->get(CONFIG_IBEACON_MAJOR, &major);
	_settings->get(CONFIG_IBEACON_MINOR, &minor);
	_settings->get(CONFIG_IBEACON_UUID, uuid.uuid128);
	_settings->get(CONFIG_IBEACON_TXPOWER, &rssi);

	//! create ibeacon object
	_beacon = new IBeacon(uuid, major, minor, rssi);

	//! create the Service Data  object which will be used
	//! to advertise certain state variables
	_serviceData = new ServiceData();

	// initialize service data
	uint8_t opMode;
	State::getInstance().get(STATE_OPERATION_MODE, opMode);

	// if we're in setup mode, do not alter the advertisement
	if (opMode != OPERATION_MODE_SETUP) {
		//! read crownstone id from storage
		uint16_t crownstoneId;
		_settings->get(CONFIG_CROWNSTONE_ID, &crownstoneId);
		LOGi("Set crownstone id to %d", crownstoneId);

		//! and set it to the service data
		_serviceData->updateCrownstoneId(crownstoneId);

		// fill service data with initial data
		uint8_t switchState;
		_stateVars->get(STATE_SWITCH_STATE, switchState);
		_serviceData->updateSwitchState(switchState);

		_serviceData->updateTemperature(getTemperature());

//		int32_t powerUsage;
//		_stateVars->get(STATE_POWER_USAGE, powerUsage);
//		_serviceData->updatePowerUsage(powerUsage);
//		int32_t accumulatedEnergy;
//		_stateVars->get(STATE_ACCUMULATED_ENERGY, accumulatedEnergy);
//		_serviceData->updateAccumulatedEnergy(accumulatedEnergy);
	}

	//! assign service data to stack
	_stack->setServiceData(_serviceData);

	if (_settings->isSet(CONFIG_IBEACON_ENABLED)) {
		_stack->configureIBeacon(_beacon, _boardsConfig.deviceType);
	} else {
		_stack->configureBleDevice(_boardsConfig.deviceType);
	}

}

void Crownstone::createSetupServices() {
	LOGi(STR_CREATE_ALL_SERVICES);

	//! should be available always
	_deviceInformationService = new DeviceInformationService();
	_stack->addService(_deviceInformationService);

	_setupService = new SetupService();
	_stack->addService(_setupService);

}

void Crownstone::createCrownstoneServices() {
	LOGi(STR_CREATE_ALL_SERVICES);

	//! should be available always
	_deviceInformationService = new DeviceInformationService();
	_stack->addService(_deviceInformationService);

	BLEutil::print_heap("Heap DeviceInformationService: ");
	BLEutil::print_stack("Stack DeviceInformationService: ");

#if CROWNSTONE_SERVICE==1
	//! should be available always
	_crownstoneService = new CrownstoneService();
	_stack->addService(_crownstoneService);
	BLEutil::print_heap("Heap CrownstoneService: ");
	BLEutil::print_stack("Stack CrownstoneService: ");
#endif

#if GENERAL_SERVICE==1
	//! general services, such as internal temperature, setting names, etc.
	_generalService = new GeneralService;
	_stack->addService(_generalService);
	BLEutil::print_heap("Heap GeneralService: ");
	BLEutil::print_stack("Stack GeneralService: ");
#endif

#if INDOOR_SERVICE==1
	//! now, build up the services and characteristics.
	_localizationService = new IndoorLocalizationService;
	_stack->addService(_localizationService);
	BLEutil::print_heap("Heap IndoorLocalizationService: ");
	BLEutil::print_stack("Stack IndoorLocalizationService: ");
#endif

#if POWER_SERVICE==1
	if (IS_CROWNSTONE(_boardsConfig.deviceType)) {
		_powerService = new PowerService;
		_stack->addService(_powerService);
	} else {
		LOGe("Power service not available");
	}
#endif

#if SCHEDULE_SERVICE==1
	_scheduleService = new ScheduleService;
	_stack->addService(_scheduleService);
#endif

}

/**
 * The default name. This can later be altered by the user if the corresponding service and characteristic is enabled.
 */
void Crownstone::setName() {
	char device_name[32];
	uint16_t size;
	_settings->get(CONFIG_NAME, device_name, size);

#if CHANGE_NAME_ON_RESET==1
	//! get reset counter
	uint16_t resetCounter;
	State::getInstance().get(STATE_RESET_COUNTER, resetCounter);
//	uint16_t minor;
//	ps_configuration_t cfg = Settings::getInstance().getConfig();
//	Storage::getUint16(cfg.beacon.minor, minor, BEACON_MINOR);
	char devicename_resetCounter[32];
	//! clip name to 5 chars and add reset counter at the end
	sprintf(devicename_resetCounter, "%.*s_%d", MIN(size, 5), device_name, resetCounter);
	std::string deviceName = std::string(devicename_resetCounter);
#else
	std::string deviceName(device_name, size);
#endif

	//! assign name
	LOGi(FMT_SET_STR_VAL, "name", deviceName.c_str());
	_stack->updateDeviceName(deviceName); //! max len = ble_gap_devname_max_len (31)
	_stack->updateAppearance(BLE_APPEARANCE_GENERIC_TAG);
}

void Crownstone::prepareNormalOperationMode() {

	LOGi(FMT_CREATE, "Timer");
	_timer->createSingleShot(_mainTimerId, (app_timer_timeout_handler_t)Crownstone::staticTick);

	//! create scanner object
	_scanner->init();
	_scanner->setStack(_stack);
	BLEutil::print_heap("Heap scanner: ");
	BLEutil::print_stack("Stack scanner: ");
//	if (_settings->isSet(CONFIG_TRACKER_ENABLED)) {
#if CHAR_TRACK_DEVICES == 1
	_tracker->init();
	BLEutil::print_heap("Heap tracker: ");
	BLEutil::print_stack("Stack tracker: ");
#endif
//	}

	//! create scheduler
	_scheduler = &Scheduler::getInstance();
	BLEutil::print_heap("Heap scheduler: ");
	BLEutil::print_stack("Stack scheduler: ");


#if BUILD_MESHING == 1
//	if (_settings->isEnabled(CONFIG_MESH_ENABLED)) {
		_mesh = &Mesh::getInstance();
		_mesh->init();
//	}
#endif
}

void Crownstone::startUp() {

	LOGi(FMT_HEADER, "startup");

	uint32_t gpregret;
	sd_power_gpregret_get(&gpregret);
	LOGi("Soft reset count: %d", gpregret);

	uint16_t bootDelay;
	_settings->get(CONFIG_BOOT_DELAY, &bootDelay);
	if (bootDelay) {
		LOGi("Boot delay: %d ms", bootDelay);
		nrf_delay_ms(bootDelay);
	}

	//! start advertising
#if EDDYSTONE==1
	_eddystone->advertising_start();
#else
	_stack->startAdvertising();
#endif
	//! have to give the stack a moment of pause to start advertising, otherwise we get into race conditions
	nrf_delay_ms(50);

	if (IS_CROWNSTONE(_boardsConfig.deviceType)) {
		//! Start switch, so it can be used.
		_switch->start();

		//! Start temperature guard regardless of operation mode
		LOGi(FMT_START, "temp guard");
		_temperatureGuard->start();

		//! Start power sampler regardless of operation mode (as it is used for the current based soft fuse)
		LOGi(FMT_START, "power sampling");
		_powerSampler->startSampling();
	}

	//! the rest we only execute if we are in normal operation
	//! during setup mode, most of the crownstone's functionality is
	//! disabled
	if (_operationMode == OPERATION_MODE_NORMAL) {

		//! start main tick
		scheduleNextTick();
		_stack->startTicking();

		if (IS_CROWNSTONE(_boardsConfig.deviceType)) {
			// Let the power sampler call the PWM callback function on zero crossings.
			_powerSampler->enableZeroCrossingInterrupt(handleZeroCrossing);

			// This way doesn't seem any better: https://stackoverflow.com/questions/2298242/callback-functions-in-c
//			_powerSampler->enableZeroCrossingInterrupt(PWM::getInstance().onZeroCrossing);
		}

		_scheduler->start();

		if (_settings->isSet(CONFIG_SCANNER_ENABLED)) {
			RNG rng;
			uint16_t delay = rng.getRandom16() / 6; // Delay in ms (about 0-10 seconds)
			_scanner->delayedStart(delay);
		}

//		if (_settings->isSet(CONFIG_TRACKER_ENABLED)) {
#if CHAR_TRACK_DEVICES == 1
			_tracker->startTracking();
#endif
//		}

		if (_settings->isSet(CONFIG_MESH_ENABLED)) {
#if BUILD_MESHING == 1
//			nrf_delay_ms(500);
			//! TODO: start with delay please
			_mesh->start();
#endif
		} else {
			LOGi("Mesh not enabled");
		}
//		BLEutil::print_heap("Heap startup: ");
//		BLEutil::print_stack("Stack startup: ");

	}

	uint32_t err_code;
	ble_gap_addr_t address;
	err_code = sd_ble_gap_address_get(&address);
	APP_ERROR_CHECK(err_code);

	log(SERIAL_INFO, "BLE Address: ");	
	BLEutil::printAddress((uint8_t*)address.addr, BLE_GAP_ADDR_LEN);

#ifdef RELAY_DEFAULT_ON
#if RELAY_DEFAULT_ON==0
	Switch::getInstance().turnOff();
#endif
#if RELAY_DEFAULT_ON==1
	Switch::getInstance().turnOn();
#endif
#endif
}

void Crownstone::tick() {
	//! Right now, this function is only called in normal operation

#if ADVERTISEMENT_IMPROVEMENT==1 // TODO: remove this macro
	//! update advertisement parameters (to improve scanning on (some) android phones)
	_stack->updateAdvertisement(true);
#endif

	//! update advertisement (to update service data)
	_stack->setAdvertisementData();

	//! update temperature
	int32_t temperature = getTemperature();
	_stateVars->set(STATE_TEMPERATURE, temperature);

	scheduleNextTick();
}

void Crownstone::scheduleNextTick() {
	Timer::getInstance().start(_mainTimerId, HZ_TO_TICKS(CROWNSTONE_UPDATE_FREQUENCY), this);
}

void Crownstone::run() {

	LOGi(FMT_HEADER, "running");

	//! forever, run scheduler, wait for events and handle them
#ifdef DEBUGGING_MESHING
	int i = 0;
#endif
	while(1) {

		app_sched_execute();

#if(NORDIC_SDK_VERSION > 5)
		BLE_CALL(sd_app_evt_wait, ());
#else
		BLE_CALL(sd_app_event_wait, () );
#endif

#ifdef DEBUGGING_MESHING
		if (i == 100) {
			LOGi("Nothing to do at t=100");
		}
		if (i == 200) {
			LOGi("Send first message to ourselves");
			
			uint32_t err_code;
			ble_gap_addr_t address;
			err_code = sd_ble_gap_address_get(&address);
			APP_ERROR_CHECK(err_code);

			mesh_message_t test_msg;
			memcpy(test_msg.header.address, address.addr, BLE_GAP_ADDR_LEN);
			test_msg.header.messageType = STATE_MESSAGE;
			test_msg.payload[0] = 66;

			MeshControl::getInstance().send(DATA_CHANNEL, (void*)&test_msg, sizeof(mesh_message_t));
		}
		if ((i % 400000) == 0) {
			LOGi("Send next message into mesh");
			mesh_message_t test_msg;
			test_msg.payload[0] = i / 400000;
			memset(test_msg.header.address, 0, BLE_GAP_ADDR_LEN);
			MeshControl::getInstance().send(DATA_CHANNEL, (void*)&test_msg, sizeof(mesh_message_t));
		}

		++i;
#endif
	}
}

void Crownstone::handleEvent(uint16_t evt, void* p_data, uint16_t length) {

//	LOGi("handleEvent: %d", evt);

	bool reconfigureBeacon = false;
	switch(evt) {

	case CONFIG_NAME: {
		_stack->updateDeviceName(std::string((char*)p_data, length));
		// need to reconfigure scan response package for updated name
		_stack->configureScanResponse(_boardsConfig.deviceType);
		_stack->setAdvertisementData();
		break;
	}
	case CONFIG_IBEACON_MAJOR: {
		_beacon->setMajor(*(uint32_t*)p_data);
		reconfigureBeacon = true;
		break;
	}
	case CONFIG_IBEACON_MINOR: {
		_beacon->setMinor(*(uint32_t*)p_data);
		reconfigureBeacon = true;
		break;
	}
	case CONFIG_IBEACON_UUID: {
		_beacon->setUUID(*(ble_uuid128_t*)p_data);
		reconfigureBeacon = true;
		break;
	}
	case CONFIG_IBEACON_TXPOWER: {
		_beacon->setTxPower(*(int8_t*)p_data);
		reconfigureBeacon = true;
		break;
	}
	case CONFIG_TX_POWER: {
//		LOGd("setTxPowerLevel %d", *(int8_t*)p_data);
		_stack->setTxPowerLevel(*(int8_t*)p_data);

		break;
	}
	case CONFIG_ADV_INTERVAL: {
		_stack->setAdvertisingInterval(*(uint32_t*)p_data);
		_stack->configureAdvertisementParameters();

		if (_stack->isAdvertising()) {
			_stack->stopAdvertising();
			_stack->startAdvertising();
		}
//		restartAdvertising = true;
		break;
	}
	case CONFIG_PASSKEY: {
		_stack->setPasskey((uint8_t*)p_data);
		break;
	}

	case EVT_ADVERTISEMENT_PAUSE: {
		if (_stack->isAdvertising()) {
			_advertisementPaused = true;
			_stack->stopAdvertising();
		}
		break;
	}
	case EVT_ADVERTISEMENT_RESUME: {
		if (_advertisementPaused) {
			_advertisementPaused = false;
			_stack->startAdvertising();
		}
		break;
	}
	case CONFIG_IBEACON_ENABLED: {
		bool enabled = *(bool*)p_data;
		if (enabled) {
			_stack->configureIBeaconAdvData(_beacon);
		} else {
			_stack->configureBleDeviceAdvData();
		}
		_stack->setAdvertisementData();
		break;
	}
	case EVT_ADVERTISEMENT_UPDATED: {
		_stack->setAdvertisementData();
		break;
	}
	case EVT_BROWNOUT_IMPENDING: {
		// turn everything off that consumes power
		LOGf("brownout impending!! force shutdown ...")

#if BUILD_MESHING == 1
		rbc_mesh_stop();
#endif
		_scanner->stop();

		if (IS_CROWNSTONE(_boardsConfig.deviceType)) {
			// [11-jul-2017] Since we store the switch state, we don't have to switch here.
//			_switch->setSwitch(0);
			_powerSampler->stopSampling();
		}

		// now reset with brownout reset mask set.
		// NOTE: do not clear the gpregret register, this way
		//   we can count the number of brownouts in the bootloader
		sd_power_gpregret_set(GPREGRET_BROWNOUT_RESET);
		// soft reset, because brownout can't be distinguished from
		// hard reset otherwise
		sd_nvic_SystemReset();
		break;
	}
	case EVT_CMD_RESET: {
		CommandHandler::getInstance().resetDelayed(GPREGRET_SOFT_RESET);
		break;
	}
	default: return;
	}

	if (reconfigureBeacon && _settings->isSet(CONFIG_IBEACON_ENABLED)) {
		_stack->setAdvertisementData();
	}
}

void on_exit(void) {
	LOGf("PROGRAM TERMINATED");
}

/**
 * If UART is enabled this will be the message printed out over a serial connection. Connectors are expensive, so UART
 * is not available in the final product.
 */
void welcome(uint8_t pinRx, uint8_t pinTx) {
	config_uart(pinRx, pinTx);

	_log(SERIAL_INFO, SERIAL_CRLF);
//	LOGi("ispr: %u", __get_IPSR());

//	BLEutil::print_heap("Heap init");
//	BLEutil::print_stack("Stack init");
	//! To have DFU, keep application limited to (BOOTLOADER_REGION_START - APPLICATION_START_CODE - DFU_APP_DATA_RESERVED)
	//! For (0x38000 - 0x1C000 - 0x400) this is 0x1BC00 (113664 bytes)
	LOGi("Welcome Crownstone");
	LOGi("Compilation date: %s", STRINGIFY(COMPILATION_DAY));
	LOGi("Compilation time: %s", __TIME__);
#ifdef GIT_HASH
	LOGi("Git hash: %s", GIT_HASH);
#else
	LOGi("Firmware version: %s", FIRMWARE_VERSION);
#endif
	LOGi("Hardware version: %s", get_hardware_version());
}

/**********************************************************************************************************************/

/**********************************************************************************************************************
 * The main function. Note that this is not the first function called! For starters, if there is a bootloader present,
 * the code within the bootloader has been processed before. But also after the bootloader, the code in
 * cs_sysNrf51.c will set event handlers and other stuff (such as coping with product anomalies, PAN), before calling
 * main. If you enable a new peripheral device, make sure you enable the corresponding event handler there as well.
 *********************************************************************************************************************/

int main() {
#ifdef TEST_PIN
	nrf_gpio_cfg_output(TEST_PIN);
	nrf_gpio_pin_toggle(TEST_PIN);
#endif

	// this enabled the hard float, without it, we get a hardfaults
	SCB->CPACR |= (3UL << 20) | (3UL << 22); __DSB(); __ISB();

	atexit(on_exit);

	uint32_t errCode;
	boards_config_t board = {};
	errCode = configure_board(&board);
	APP_ERROR_CHECK(errCode);

	// Init gpio pins early in the process!
	if (IS_CROWNSTONE(board.deviceType)) {
		nrf_gpio_cfg_output(board.pinGpioPwm);
		if (board.flags.pwmInverted) {
			nrf_gpio_pin_set(board.pinGpioPwm);
		} else {
			nrf_gpio_pin_clear(board.pinGpioPwm);
		}
		//! Relay pins
		if (board.flags.hasRelay) {
			nrf_gpio_cfg_output(board.pinGpioRelayOff);
			nrf_gpio_pin_clear(board.pinGpioRelayOff);
			nrf_gpio_cfg_output(board.pinGpioRelayOn);
			nrf_gpio_pin_clear(board.pinGpioRelayOn);
		}
	}

	//! int uart, be nice and say hello
#ifdef TEST_PIN
	nrf_gpio_pin_toggle(TEST_PIN);
#endif
	welcome(board.pinGpioRx, board.pinGpioTx);

	BLEutil::print_heap("Heap welcome: ");
	BLEutil::print_stack("Stack welcome: ");

#ifdef TEST_PIN
	nrf_gpio_pin_toggle(TEST_PIN);
#endif
	Crownstone crownstone(board); // 250 ms

	BLEutil::print_heap("Heap crownstone construct: ");
	BLEutil::print_stack("Stack crownstone construct: ");

	// initialize crownstone (depends on the operation mode) ...
#ifdef TEST_PIN
	nrf_gpio_pin_toggle(TEST_PIN);
#endif
	crownstone.init(); // 13 ms

	BLEutil::print_heap("Heap crownstone init: ");
	BLEutil::print_stack("Stack crownstone init: ");

	//! start up phase, start ticking (depends on the operation mode) ...
#ifdef TEST_PIN
	nrf_gpio_pin_toggle(TEST_PIN);
#endif
	crownstone.startUp(); // 500 ms

	//! run forever ...
#ifdef TEST_PIN
	nrf_gpio_pin_toggle(TEST_PIN);
#endif
	crownstone.run();

}
