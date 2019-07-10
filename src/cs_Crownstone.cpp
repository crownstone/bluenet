/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 14 Aug., 2014
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

/**********************************************************************************************************************
 *
 * The Crownstone is a high-voltage (110-240V) switch. It can be used for indoor localization and building automation.
 * It is an essential building block for smart homes. Contemporary switches do come with smartphone apps. However,
 * this does not simplify their use. The user needs to unlock her phone, open the app, navigate to the right screen,
 * and click a particular icon. A truly smart home knows that the user wants the lights on. For this, indoor
 * localization on a room level is required. Now, a smart home can turn on the lights there were the user resides.
 * Indoor localization can be used to adjust lights, temperature, or even music to your presence.
 *
 * Very remarkable, our Crownstone technology is one of the first open-source Internet of Things devices entering the
 * market (2016). Our firmware dates from 2014. Please, contact us if you encounter other IoT devices that are open
 * source!
 *
 * Read more on: https://crownstone.rocks
 *
 * Crownstone uses quite a sophisticated build system. It has to account for multiple devices and multiple
 * configuration options. For that the CMake system is used in parallel. Configuration options are set in a file
 * called CMakeBuild.config. Details can be found in the following documents:
 *
 * - Development/Installation:         https://github.com/crownstone/bluenet/blob/master/docs/INSTALL.md
 * - Protocol Definition:              https://github.com/crownstone/bluenet/blob/master/docs/PROTOCOL.md
 * - Firmware Specifications:          https://github.com/crownstone/bluenet/blob/master/docs/FIRMWARE_SPECS.md
 *
 *********************************************************************************************************************/

#include <ble/cs_CrownstoneManufacturer.h>
#include <cfg/cs_Boards.h>
#include <cfg/cs_HardwareVersions.h>
#include <cs_Crownstone.h>
#include <common/cs_Types.h>
#include <drivers/cs_PWM.h>
#include <drivers/cs_RNG.h>
#include <drivers/cs_RTC.h>
#include <drivers/cs_Temperature.h>
#include <drivers/cs_Timer.h>
#include <events/cs_EventDispatcher.h>
#include <processing/cs_EncryptionHandler.h>
#include <protocol/cs_UartProtocol.h>
#include <storage/cs_State.h>
#include <structs/buffer/cs_MasterBuffer.h>
#include <structs/buffer/cs_EncryptionBuffer.h>
#include <util/cs_Utils.h>

extern "C" {
#include <nrf_nvmc.h>
}

// Define test pin to enable gpio debug.
#define TEST_PIN 18

// Define to enable leds. WARNING: this is stored in UICR and not easily reversible!
//#define ENABLE_LEDS

//#define ANNE_TEST_FACTORY_RESET

/**********************************************************************************************************************
 * Main functionality
 *********************************************************************************************************************/

void handleZeroCrossing() {
	PWM::getInstance().onZeroCrossing();
}

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
 *  + watchdog
 *
 * The initialization is done in separate function.
 */
Crownstone::Crownstone(boards_config_t& board) :
	_boardsConfig(board),
	_mainTimerId(NULL),
	_operationMode(OperationMode::OPERATION_MODE_UNINITIALIZED)
{
	// TODO: can be replaced by: APP_TIMER_DEF(_mainTimerId); Though that makes _mainTimerId a static variable.
	_mainTimerData = { {0} };
	_mainTimerId = &_mainTimerData;

	MasterBuffer::getInstance().alloc(MASTER_BUFFER_SIZE);
	EncryptionBuffer::getInstance().alloc(BLE_GATTS_VAR_ATTR_LEN_MAX);
	EventDispatcher::getInstance().addListener(this);

	_stack = &Stack::getInstance();
	_timer = &Timer::getInstance();
	_storage = &Storage::getInstance();
	_state = &State::getInstance();
	_commandHandler = &CommandHandler::getInstance();
	_factoryReset = &FactoryReset::getInstance();

	_scanner = &Scanner::getInstance();
#if BUILD_MESHING == 1
	_mesh = &Mesh::getInstance();
#endif

	if (IS_CROWNSTONE(_boardsConfig.deviceType)) {
		_switch = &Switch::getInstance();
		_temperatureGuard = &TemperatureGuard::getInstance();
		_powerSampler = &PowerSampling::getInstance();
		_watchdog = &Watchdog::getInstance();
	}

};

/**
 * Initialize Crownstone firmware. First drivers are initialized (log modules, storage modules, ADC conversion,
 * timers). Then everything is configured independent of the mode (everything that is common to whatever mode the
 * Crownstone runs on). A callback to the local staticTick function for a timer is set up. Then the mode of
 * operation is switched and the BLE services are initialized.
 */
void Crownstone::init(uint16_t step) {
	switch (step) {
	case 0: {
		LOGi(FMT_HEADER, "init");
		initDrivers(step);
		break;
	}
	case 1: {
		initDrivers(step);
		LOG_MEMORY;
		LOG_FLUSH();

		TYPIFY(STATE_OPERATION_MODE) mode;
		_state->get(CS_TYPE::STATE_OPERATION_MODE, &mode, sizeof(mode));
		_operationMode = getOperationMode(mode);

		//! configure the crownstone
		LOGi(FMT_HEADER, "configure");
		configure();
		LOG_FLUSH();

		LOGi(FMT_CREATE, "timer");
		_timer->createSingleShot(_mainTimerId, (app_timer_timeout_handler_t)Crownstone::staticTick);
		LOG_FLUSH();

		LOGi(FMT_HEADER, "mode");
		switchMode(_operationMode);
		LOG_FLUSH();

		LOGi(FMT_HEADER, "init services");

		_stack->initServices();
		LOG_FLUSH();
		break;
	}
	}
}

/**
 * This must be called after the SoftDevice has started. The order in which things should be initialized is as follows:
 *   1. Stack.               Starts up the softdevice. It controls a lot of devices, so need to set it early.
 *   2. Timer.
 *   3. Storage.             Definitely after the stack has been initialized.
 *   4. State.               Storage should be initialized here.
 */
void Crownstone::initDrivers(uint16_t step) {
	switch (step) {
	case 0: {
		LOGi("Init drivers");
		_stack->init();
		_timer->init();

		_stack->initSoftdevice();

		_storage->init();
		break;
	}
	case 1: {
		_state->init(&_boardsConfig);

		// If not done already, init UART
		// TODO: make into a class with proper init() function
		if (!_boardsConfig.flags.hasSerial) {
			serial_config(_boardsConfig.pinGpioRx, _boardsConfig.pinGpioTx);
			TYPIFY(CONFIG_UART_ENABLED) uartEnabled;
			_state->get(CS_TYPE::CONFIG_UART_ENABLED, &uartEnabled, sizeof(uartEnabled));
			serial_enable((serial_enable_t)uartEnabled);
		}

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
		}

		// init GPIOs
		if (_boardsConfig.flags.hasLed) {
			LOGi("Configure LEDs");
			// Note: DO NOT USE THEM WHILE SCANNING OR MESHING
			nrf_gpio_cfg_output(_boardsConfig.pinLedRed);
			nrf_gpio_cfg_output(_boardsConfig.pinLedGreen);
			// Turn the leds off
			if (_boardsConfig.flags.ledInverted) {
				nrf_gpio_pin_set(_boardsConfig.pinLedRed);
				nrf_gpio_pin_set(_boardsConfig.pinLedGreen);
			}
			else {
				nrf_gpio_pin_clear(_boardsConfig.pinLedRed);
				nrf_gpio_pin_clear(_boardsConfig.pinLedGreen);
			}
		}
		break;
	}
	}
}

/**
 * Configure the Bluetooth stack. This also increments the reset counter.
 *
 * The order within this function is important. For example setName() sets the BLE device name and
 * configureAdvertisements() defines advertisement parameters on appearance. These have to be called after
 * the storage has been initialized.
 */
void Crownstone::configure() {
	assert(_stack != NULL, "Stack");
	assert(_storage != NULL, "Storage");

	LOGi("> stack ...");

	_stack->initRadio();

	configureStack();

	// Don't do garbage collection now, it will block reading flash.
//	_storage->garbageCollect();

	increaseResetCounter();

	setName();

	LOGi("> advertisement ...");
	configureAdvertisement();
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
	// Set the stored advertisement interval
	TYPIFY(CONFIG_ADV_INTERVAL) advInterval;
	_state->get(CS_TYPE::CONFIG_ADV_INTERVAL, &advInterval, sizeof(advInterval));
	_stack->setAdvertisingInterval(advInterval);

	// Set callback handler for a connection event
	_stack->onConnect([&](uint16_t conn_handle) {
		LOGi("onConnect...");
		// TODO: see https://devzone.nordicsemi.com/index.php/about-rssi-of-ble
		// be neater about it... we do not need to stop, only after a disconnect we do...
#if ENABLE_RSSI_FOR_CONNECTION==1
		sd_ble_gap_rssi_stop(conn_handle);
		sd_ble_gap_rssi_start(conn_handle, 0, 0);
#endif
		uint32_t gpregret_id = 0;
		uint32_t gpregret_msk = 0xFF;
		sd_power_gpregret_clr(gpregret_id, gpregret_msk);

		// Can't advertise connectable advertisements when already connected.
		// TODO: move this code to stack?
		_stack->setNonConnectable();
		_stack->restartAdvertising();
	});

	// Set callback handler for a disconnection event
	_stack->onDisconnect([&](uint16_t conn_handle) {
		LOGi("onDisconnect...");
		if (_operationMode == OperationMode::OPERATION_MODE_SETUP) {
//			_stack->changeToLowTxPowerMode();
			_stack->changeToNormalTxPowerMode();
		}

		// TODO: move this code to stack?
		_stack->setConnectable();
		_stack->restartAdvertising();
	});
}

/**
 * Populate advertisement (including service data) with information. The persistence mode is obtained from storage
 * (the _operationMode var is not used).
 */
void Crownstone::configureAdvertisement() {
	// Create the iBeacon parameter object which will be used to configure the advertisement as an iBeacon.
	TYPIFY(CONFIG_IBEACON_MAJOR) major;
	TYPIFY(CONFIG_IBEACON_MINOR) minor;
	TYPIFY(CONFIG_IBEACON_TXPOWER) rssi;
	ble_uuid128_t uuid;
//	if (_operationMode == OperationMode::OPERATION_MODE_NORMAL) {
		_state->get(CS_TYPE::CONFIG_IBEACON_MAJOR, &major, sizeof(major));
		_state->get(CS_TYPE::CONFIG_IBEACON_MINOR, &minor, sizeof(minor));
		_state->get(CS_TYPE::CONFIG_IBEACON_UUID, uuid.uuid128, sizeof(uuid.uuid128));
		_state->get(CS_TYPE::CONFIG_IBEACON_TXPOWER, &rssi, sizeof(rssi));
//	}
//	else {
//		// TODO: Get default!
//	}
	LOGd("iBeacon: major=%u, minor=%u, rssi_on_1m=%i", major, minor, rssi);

	_beacon = new IBeacon(uuid, major, minor, rssi);

	// Create the ServiceData object which will be (mis)used to advertise select state variables from the Crownstone.
	_serviceData = new ServiceData();
	_serviceData->setDeviceType(_boardsConfig.deviceType);
	_serviceData->init();

	// The service data is populated with State information, but only in NORMAL mode.
	if (_operationMode == OperationMode::OPERATION_MODE_NORMAL) {
		LOGd("Normal mode, fill with state info");

		// Write crownstone id to the service data object.
		TYPIFY(CONFIG_CROWNSTONE_ID) crownstoneId;
		_state->get(CS_TYPE::CONFIG_CROWNSTONE_ID, &crownstoneId, sizeof(crownstoneId));
		_serviceData->updateCrownstoneId(crownstoneId);
		LOGi("Set crownstone id to %u", crownstoneId);

		// Write switch state to the service data object.
		TYPIFY(STATE_SWITCH_STATE) switchState;
		_state->get(CS_TYPE::STATE_SWITCH_STATE, &switchState, sizeof(switchState));
		_serviceData->updateSwitchState(switchState.asInt);

		// Write temperature to the service data object.
		_serviceData->updateTemperature(getTemperature());
	}

	// assign service data to stack
	_stack->setServiceData(_serviceData);

	if (_state->isTrue(CS_TYPE::CONFIG_IBEACON_ENABLED)) {
		LOGd("Configure iBeacon");
		_stack->configureIBeacon(_beacon, _boardsConfig.deviceType);
	}
	else {
		LOGd("Configure BLE device");
		_stack->configureBleDevice(_boardsConfig.deviceType);
	}

}

/**
 * Create a particular service. Depending on the mode we can choose to create a set of services that we would need.
 * After creation of a service it cannot be deleted. This is a restriction of the Nordic Softdevice. If you need a
 * new set of services, you will need to change the mode of operation (in a persisted field). Then you can restart
 * the device and use the mode to enable another set of services.
 */
void Crownstone::createService(const ServiceEvent event) {
	switch(event) {
		case CREATE_DEVICE_INFO_SERVICE:
			LOGd("Create device info service");
			_deviceInformationService = new DeviceInformationService();
			_stack->addService(_deviceInformationService);
			break;
		case CREATE_SETUP_SERVICE:
			_setupService = new SetupService();
			_stack->addService(_setupService);
			break;
		case CREATE_CROWNSTONE_SERVICE:
			_crownstoneService = new CrownstoneService();
			_stack->addService(_crownstoneService);
			break;
		default:
			LOGe("Unknown creation event");
	}
}

/** Switch from one operation mode to another.
 *
 * Depending on the operation mode we have a different set of services / characteristics enabled. Subsequently,
 * also different entities are started (for example a scanner, or the BLE mesh).
 */
void Crownstone::switchMode(const OperationMode & newMode) {

	LOGd("Current mode: %s", TypeName(_oldOperationMode));
	LOGd("Switch to mode: %s", TypeName(newMode));

	switch(_oldOperationMode) {
		case OperationMode::OPERATION_MODE_UNINITIALIZED:
			break;
		case OperationMode::OPERATION_MODE_DFU:
		case OperationMode::OPERATION_MODE_NORMAL:
		case OperationMode::OPERATION_MODE_SETUP:
		case OperationMode::OPERATION_MODE_FACTORY_RESET:
			LOGe("Only switching from UNINITIALIZED to another mode is supported");
			break;
		default:
			LOGe("Unknown mode %i!", newMode);
			return;
	}

	_stack->halt();

	// Remove services that belong to the current operation mode.
	// This is not done... It is impossible to remove services in the SoftDevice.

	// Start operation mode
	startOperationMode(newMode);

	// Create services that belong to the new mode.
	switch(newMode) {
		case OperationMode::OPERATION_MODE_NORMAL:
			if (_oldOperationMode == OperationMode::OPERATION_MODE_UNINITIALIZED) {
				createService(CREATE_DEVICE_INFO_SERVICE);
			}
			createService(CREATE_CROWNSTONE_SERVICE);
			break;
		case OperationMode::OPERATION_MODE_SETUP:
			if (_oldOperationMode == OperationMode::OPERATION_MODE_UNINITIALIZED) {
				createService(CREATE_DEVICE_INFO_SERVICE);
			}
			createService(CREATE_SETUP_SERVICE);
			break;
		default:
			// nothing to do
			;
	}

	// Loop through all services added to the stack and create the characteristics.
	_stack->createCharacteristics();

	_stack->resume();

	switch(newMode) {
		case OperationMode::OPERATION_MODE_SETUP:
			LOGd("Configure setup mode");
//			_stack->changeToLowTxPowerMode();
			_stack->changeToNormalTxPowerMode();
			break;
		case OperationMode::OPERATION_MODE_FACTORY_RESET:
			LOGd("Configure factory reset mode");
			FactoryReset::getInstance().finishFactoryReset(_boardsConfig.deviceType);
			_stack->changeToNormalTxPowerMode();
			break;
		case OperationMode::OPERATION_MODE_DFU:
			LOGd("Configure DFU mode");
			// TODO: have this function somewhere else.
			CommandHandler::getInstance().handleCommand(CTRL_CMD_GOTO_DFU);
			_stack->changeToNormalTxPowerMode();
			break;
		default:
			_stack->changeToNormalTxPowerMode();
	}

	// Enable AES encryption.
	if (_state->isTrue(CS_TYPE::CONFIG_ENCRYPTION_ENABLED)) {
		LOGi(FMT_ENABLE, "AES encryption");
		_stack->setAesEncrypted(true);
	}

//	_operationMode = newMode;
}

/**
 * The default name. This can later be altered by the user if the corresponding service and characteristic is enabled.
 * It is loaded from memory or from the default and written to the Stack.
 */
void Crownstone::setName() {
	static bool addResetCounterToName = false;
#if CHANGE_NAME_ON_RESET==1
	addResetCounterToName = true;
#endif
//	TYPIFY(CONFIG_NAME)
	char device_name[32];
	cs_state_data_t stateNameData(CS_TYPE::CONFIG_NAME, (uint8_t*)device_name, sizeof(device_name));
	_state->get(stateNameData);
	std::string deviceName;
	if (addResetCounterToName) {
		//! clip name to 5 chars and add reset counter at the end
		TYPIFY(STATE_RESET_COUNTER) resetCounter;
		_state->get(CS_TYPE::STATE_RESET_COUNTER, &resetCounter, sizeof(resetCounter));
		char devicename_resetCounter[32];
		sprintf(devicename_resetCounter, "%.*s_%d", MIN(stateNameData.size, 5), device_name, resetCounter);
		deviceName = std::string(devicename_resetCounter);
	} else {
		deviceName = std::string(device_name, stateNameData.size);
	}
	_stack->updateDeviceName(deviceName);
}

/**
 * Start the different modules depending on the operational mode. For example, in normal mode we use a scanner and
 * the mesh. In setup mode we use the serial module (but only RX).
 */
void Crownstone::startOperationMode(const OperationMode & mode) {
	switch(mode) {
		case OperationMode::OPERATION_MODE_NORMAL:
			_scanner->init();
			_scanner->setStack(_stack);
			_scheduler = &Scheduler::getInstance();

#if BUILD_MESHING == 1
			if (_state->isTrue(CS_TYPE::CONFIG_MESH_ENABLED)) {
				_mesh->init();
			}
#endif
			EncryptionHandler::getInstance().RC5InitKey(EncryptionAccessLevel::BASIC); // BackgroundAdvertisementHandler needs RC5.
			_commandAdvHandler = &CommandAdvHandler::getInstance();
			_commandAdvHandler->init();
			break;
		case OperationMode::OPERATION_MODE_SETUP:
			// TODO: Why this hack?
			if (serial_get_state() == SERIAL_ENABLE_NONE) {
				serial_enable(SERIAL_ENABLE_RX_ONLY);
			}
			break;
		default:
			// nothing to be done
			;
	}
}

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
void Crownstone::startUp() {

	LOGi(FMT_HEADER, "startup");

	uint32_t gpregret_id = 0;
	uint32_t gpregret;
	sd_power_gpregret_get(gpregret_id, &gpregret);
	LOGi("Content gpregret register: %d", gpregret);

	TYPIFY(CONFIG_BOOT_DELAY) bootDelay;
	_state->get(CS_TYPE::CONFIG_BOOT_DELAY, &bootDelay, sizeof(bootDelay));
	if (bootDelay) {
		LOGi("Boot delay: %d ms", bootDelay);
		nrf_delay_ms(bootDelay);
	}

	//! start advertising
	LOGi("Start advertising");
	_stack->startAdvertising();

	// Set the stored tx power, must be done after advertising has started.
	TYPIFY(CONFIG_TX_POWER) txPower = 0;
	_state->get(CS_TYPE::CONFIG_TX_POWER, &txPower, sizeof(txPower));
	_stack->setTxPowerLevel(txPower);

	// Have to give the stack a moment of pause to start advertising, otherwise we get into race conditions.
	// TODO: Is this still the case? Can we solve this differently?
	nrf_delay_ms(50);

	if (IS_CROWNSTONE(_boardsConfig.deviceType)) {
		//! Start switch, so it can be used.
		_switch->start();

		if (_operationMode == OperationMode::OPERATION_MODE_SETUP &&
				_boardsConfig.deviceType == DEVICE_CROWNSTONE_BUILTIN) {
			_switch->delayedSwitch(SWITCH_ON, SWITCH_ON_AT_SETUP_BOOT_DELAY);
		}

		//! Start temperature guard regardless of operation mode
		LOGi(FMT_START, "temp guard");
		_temperatureGuard->start();

		//! Start power sampler regardless of operation mode (as it is used for the current based soft fuse)
		LOGi(FMT_START, "power sampling");
		_powerSampler->startSampling();
	}

	// Start ticking main and services.
	scheduleNextTick();

	// The rest we only execute if we are in normal operation.
	// During other operation modes, most of the crownstone's functionality is disabled.
	if (_operationMode == OperationMode::OPERATION_MODE_NORMAL) {

		if (IS_CROWNSTONE(_boardsConfig.deviceType)) {
			// Let the power sampler call the PWM callback function on zero crossings.
			_powerSampler->enableZeroCrossingInterrupt(handleZeroCrossing);
		}

		_scheduler->start();

		if (_state->isTrue(CS_TYPE::CONFIG_SCANNER_ENABLED)) {
			RNG rng;
			uint16_t delay = rng.getRandom16() / 6; // Delay in ms (about 0-10 seconds)
			_scanner->delayedStart(delay);
		}

		if (_state->isTrue(CS_TYPE::CONFIG_MESH_ENABLED)) {
#if BUILD_MESHING == 1
//			nrf_delay_ms(500);
			//! TODO: start with delay please
			_mesh->start();
			_mesh->advertise(_beacon);
#endif
		} else {
			LOGi("Mesh not enabled");
		}

	}

	uint32_t err_code;
	ble_gap_addr_t address;
	err_code = sd_ble_gap_addr_get(&address);
	APP_ERROR_CHECK(err_code);

	_log(SERIAL_INFO, "\r\n");
	_log(SERIAL_INFO, "\t\t\tAddress: ");
	BLEutil::printAddress((uint8_t*)address.addr, BLE_GAP_ADDR_LEN);
	_log(SERIAL_INFO, "\r\n");

	_state->startWritesToFlash();

#ifdef RELAY_DEFAULT_ON
#if RELAY_DEFAULT_ON==0
	Switch::getInstance().turnOff();
#endif
#if RELAY_DEFAULT_ON==1
	Switch::getInstance().turnOn();
#endif
#endif
}

/**
 * Increase the reset counter. This can be used for debugging purposes. The reset counter is written to FLASH and
 * persists over reboots.
 */
void Crownstone::increaseResetCounter() {
	TYPIFY(STATE_RESET_COUNTER) resetCounter = 0;
	_state->get(CS_TYPE::STATE_RESET_COUNTER, &resetCounter, sizeof(resetCounter));
	resetCounter++;
	LOGi("Reset counter at %u", resetCounter);
	_state->set(CS_TYPE::STATE_RESET_COUNTER, &resetCounter, sizeof(resetCounter));
}

/**
 * Operations that are not sensitive with respect to time and only need to be called at regular intervals.
 * TODO: describe function calls and why they are required.
 */
void Crownstone::tick() {
	if (_tickCount % (60*1000/TICK_INTERVAL_MS) == 0) {
		LOG_MEMORY; // To check for memory leaks
	}
	// TODO: warning when close to out of memory
	// TODO: maybe detect memory leaks?

	if (_tickCount % (500/TICK_INTERVAL_MS) == 0) {
		TYPIFY(STATE_TEMPERATURE) temperature = getTemperature();
		_state->set(CS_TYPE::STATE_TEMPERATURE, &temperature, sizeof(temperature));
	}

	// Update advertisement parameter (only in operation mode NORMAL)
	if (_tickCount % (500/TICK_INTERVAL_MS) == 0 && _operationMode == OperationMode::OPERATION_MODE_NORMAL) {
		// update advertisement parameters (to improve scanning on (some) android phones)
		_stack->updateAdvertisement(true);
		// update advertisement (to update service data)
		_stack->setAdvertisementData();
	}

	// Check for timeouts
	if (_operationMode == OperationMode::OPERATION_MODE_NORMAL) {
		if ((PWM_BOOT_DELAY_MS > 0) && (RTC::getCount() > RTC::msToTicks(PWM_BOOT_DELAY_MS))) {
			_switch->startPwm();
		}
	}

	event_t event(CS_TYPE::EVT_TICK, &_tickCount, sizeof(_tickCount));
	EventDispatcher::getInstance().dispatch(event);
	++_tickCount;

	scheduleNextTick();
}

void Crownstone::scheduleNextTick() {
	Timer::getInstance().start(_mainTimerId, MS_TO_TICKS(TICK_INTERVAL_MS), this);
}

/**
 * An infinite loop in which the application ceases control to the SoftDevice at regular times. It runs the scheduler,
 * waits for events, and handles them. Also the printed statements in the log module are flushed.
 */
void Crownstone::run() {

	LOGi(FMT_HEADER, "running");

	while(1) {
		app_sched_execute();
#if BUILD_MESHING == 1
		// See mesh_interrupt_priorities.md
		bool done = nrf_mesh_process();
		if (done) {
			sd_app_evt_wait();
		}
#else
		sd_app_evt_wait();
#endif
		LOG_FLUSH();
	}
}

/**
 * Handle events that can come from other parts of the Crownstone firmware and even originate from outside of the
 * firmware via the BLE interface (an application, or the mesh).
 */
void Crownstone::handleEvent(event_t & event) {

	switch(event.type) {
		case CS_TYPE::EVT_STORAGE_INITIALIZED:
			init(1);
			startUp();
			LOG_FLUSH();
			break;
		default:
			LOGnone("Event: %s [%i]", TypeName(event.type), to_underlying_type(event.type));
	}

	bool reconfigureBeacon = false;
	switch(event.type) {

		case CS_TYPE::CONFIG_NAME: {
			_stack->updateDeviceName(std::string((char*)event.data, event.size));
			_stack->configureScanResponse(_boardsConfig.deviceType);
			_stack->setAdvertisementData();
			break;
		}
		case CS_TYPE::CONFIG_IBEACON_MAJOR: {
			_beacon->setMajor(*(TYPIFY(CONFIG_IBEACON_MAJOR)*)event.data);
			reconfigureBeacon = true;
			break;
		}
		case CS_TYPE::CONFIG_IBEACON_MINOR: {
			_beacon->setMinor(*(TYPIFY(CONFIG_IBEACON_MINOR)*)event.data);
			reconfigureBeacon = true;
			break;
		}
		case CS_TYPE::CONFIG_IBEACON_UUID: {
			_beacon->setUUID(*(ble_uuid128_t*)event.data);
			reconfigureBeacon = true;
			break;
		}
		case CS_TYPE::CONFIG_IBEACON_TXPOWER: {
			_beacon->setTxPower(*(TYPIFY(CONFIG_IBEACON_TXPOWER)*)event.data);
			reconfigureBeacon = true;
			break;
		}
		case CS_TYPE::CONFIG_TX_POWER: {
			_stack->setTxPowerLevel(*(TYPIFY(CONFIG_TX_POWER)*)event.data);
			break;
		}
		case CS_TYPE::CONFIG_ADV_INTERVAL: {
			_stack->updateAdvertisingInterval(*(TYPIFY(CONFIG_ADV_INTERVAL)*)event.data, true);
			break;
		}
		case CS_TYPE::CMD_ENABLE_ADVERTISEMENT: {
			TYPIFY(CMD_ENABLE_ADVERTISEMENT) enable = *(TYPIFY(CMD_ENABLE_ADVERTISEMENT)*)event.data;
			if (enable) {
				_stack->startAdvertising();
			}
			else {
				_stack->stopAdvertising();
			}
			// TODO: should be done via event.
			UartProtocol::getInstance().writeMsg(UART_OPCODE_TX_ADVERTISEMENT_ENABLED, (uint8_t*)&enable, 1);
			break;
		}
		case CS_TYPE::CMD_ENABLE_MESH: {
#if BUILD_MESHING == 1
			uint8_t enable = *(uint8_t*)event.data;
			if (enable) {
				_mesh->start();
			}
			else {
				_mesh->stop();
			}
			UartProtocol::getInstance().writeMsg(UART_OPCODE_TX_MESH_ENABLED, &enable, 1);
#endif
			break;
		}
		case CS_TYPE::CONFIG_IBEACON_ENABLED: {
			TYPIFY(CONFIG_IBEACON_ENABLED) enabled = *(TYPIFY(CONFIG_IBEACON_ENABLED)*)event.data;
			if (enabled) {
				_stack->configureIBeaconAdvData(_beacon);
			} else {
				_stack->configureBleDeviceAdvData();
			}
			_stack->setAdvertisementData();
			break;
		}
		case CS_TYPE::EVT_ADVERTISEMENT_UPDATED: {
			_stack->setAdvertisementData();
			break;
		}
		case CS_TYPE::EVT_BROWNOUT_IMPENDING: {
			// turn everything off that consumes power
			LOGf("brownout impending!! force shutdown ...")

#if BUILD_MESHING == 1
			_mesh->stop();
#endif
			_scanner->stop();

			if (IS_CROWNSTONE(_boardsConfig.deviceType)) {
				//	_powerSampler->stopSampling();
			}

			uint32_t gpregret_id = 0;
			uint32_t gpregret_msk = GPREGRET_BROWNOUT_RESET;
			// now reset with brownout reset mask set.
			// NOTE: do not clear the gpregret register, this way
			//   we can count the number of brownouts in the bootloader
			sd_power_gpregret_set(gpregret_id, gpregret_msk);
			// soft reset, because brownout can't be distinguished from
			// hard reset otherwise
			sd_nvic_SystemReset();
			break;
		}
		case CS_TYPE::CMD_SET_OPERATION_MODE: {
			OperationMode mode = *(OperationMode*)event.data;
			switchMode(mode);
			break;
		}
		default:
			return;
	}

	if (reconfigureBeacon && _state->isTrue(CS_TYPE::CONFIG_IBEACON_ENABLED)) {
		_stack->setAdvertisementData();
	}
}

void on_exit(void) {
	LOGf("PROGRAM TERMINATED");
}

/**
 * If UART is enabled this will be the message printed out over a serial connection. In release mode we will not by
 * default use the UART, it will need to be turned on.
 *
 * For DFU, application should be at (BOOTLOADER_REGION_START - APPLICATION_START_CODE - DFU_APP_DATA_RESERVED). For
 * example, for (0x38000 - 0x1C000 - 0x400) this is 0x1BC00 (113664 bytes).
 */
void welcome(uint8_t pinRx, uint8_t pinTx) {
	serial_config(pinRx, pinTx);
	serial_init(SERIAL_ENABLE_RX_AND_TX);
	_log(SERIAL_INFO, SERIAL_CRLF);

#ifdef GIT_HASH
#undef FIRMWARE_VERSION
#define FIRMWARE_VERSION GIT_HASH
#endif

	LOGi("Welcome! Bluenet firmware, version %s", FIRMWARE_VERSION);
	LOGi("\033[35;1m");
	LOGi(" _|_|_|    _|                                            _|     ");
	LOGi(" _|    _|  _|  _|    _|    _|_|    _|_|_|      _|_|    _|_|_|_| ");
	LOGi(" _|_|_|    _|  _|    _|  _|_|_|_|  _|    _|  _|_|_|_|    _|     ");
	LOGi(" _|    _|  _|  _|    _|  _|        _|    _|  _|          _|     ");
	LOGi(" _|_|_|    _|    _|_|_|    _|_|_|  _|    _|    _|_|_|      _|_| ");
	LOGi("\033[0m");

	LOGi("Compilation date: %s", COMPILATION_DAY);
	LOGi("Compilation time: %s", __TIME__);
	LOGi("Hardware version: %s", get_hardware_version());
	LOGi("Verbosity: %i", SERIAL_VERBOSITY);
	LOG_MEMORY;
}

/**********************************************************************************************************************/

/** Overwrite the hardware version.
 *
 * The firmware is compiled with particular defaults. When a particular product comes from the factory line it has
 * by default FFFF FFFF in this UICR location. If this is the case, there are two options to cope with this:
 *   1. Create a custom firmware per device type where this field is adjusted at runtime.
 *   2. Create a custom firmware per device type with the UICR field state in the .hex file. In the latter case,
 *      if the UICR fields are already set, this might lead to a conflict.
 * There is chosen for the first option. Even if rare cases where there are devices types with FFFF FFFF in the field,
 * the runtime always tries to overwrite it with the (let's hope) proper state.
 */
void overwrite_hardware_version() {
	uint32_t hardwareBoard = NRF_UICR->CUSTOMER[UICR_BOARD_INDEX];
	if (hardwareBoard == 0xFFFFFFFF) {
		LOGw("Write board type into UICR");
		nrf_nvmc_write_word(HARDWARE_BOARD_ADDRESS, DEFAULT_HARDWARE_BOARD);
	}
	LOGd("Board: %p", hardwareBoard);
}

/**********************************************************************************************************************
 * The main function. Note that this is not the first function called! For starters, if there is a bootloader present,
 * the code within the bootloader has been processed before. But also after the bootloader, the code in
 * cs_sysNrf51.c will set event handlers and other stuff (such as coping with product anomalies, PAN), before calling
 * main. If you enable a new peripheral device, make sure you enable the corresponding event handler there as well.
 *********************************************************************************************************************/

int main() {
#ifdef TEST_PIN
	nrf_gpio_cfg_output(TEST_PIN);
	nrf_gpio_pin_clear(TEST_PIN);
#endif

	// this enabled the hard float, without it, we get a hardfault
	SCB->CPACR |= (3UL << 20) | (3UL << 22); __DSB(); __ISB();

	atexit(on_exit);

#if CS_SERIAL_NRF_LOG_ENABLED > 0
	NRF_LOG_INIT(NULL);
	NRF_LOG_DEFAULT_BACKENDS_INIT();
	NRF_LOG_INFO("Main");
	NRF_LOG_FLUSH();
#endif

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

	if (board.flags.hasSerial) {
		// init uart, be nice and say hello
		welcome(board.pinGpioRx, board.pinGpioTx);
		LOG_FLUSH();
	}

	Crownstone crownstone(board); // 250 ms

	// initialize crownstone (depends on the operation mode) ...

#ifdef ENABLE_LEDS
	// WARNING: this is stored in UICR and not easily reversible!
	if (NRF_UICR->NFCPINS != 0) {
		LOGw("enable gpio LEDs");
		nrf_nvmc_write_word((uint32_t)&(NRF_UICR->NFCPINS), 0);
	}
#endif
	LOGd("NFC pins: %p", NRF_UICR->NFCPINS);
	LOG_FLUSH();

	overwrite_hardware_version();

	// init drivers, configure(), create services and chars,
	crownstone.init(0);
	LOG_FLUSH();

	// run forever ...
	crownstone.run();

	return 0;
}
