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

#include <behaviour/cs_BehaviourHandler.h>
#include <behaviour/cs_BehaviourStore.h>
#include <cfg/cs_AutoConfig.h>
#include <cfg/cs_Boards.h>
#include <cfg/cs_DeviceTypes.h>
#include <cfg/cs_Git.h>
#include <cfg/cs_HardwareVersions.h>
#include <common/cs_Types.h>
#include <cs_Crownstone.h>
#include <drivers/cs_GpRegRet.h>
#include <drivers/cs_PWM.h>
#include <drivers/cs_RNG.h>
#include <drivers/cs_RTC.h>
#include <drivers/cs_Temperature.h>
#include <drivers/cs_Timer.h>
#include <drivers/cs_Watchdog.h>
#include <encryption/cs_ConnectionEncryption.h>
#include <encryption/cs_RC5.h>
#include <ipc/cs_IpcRamData.h>
#include <logging/cs_CLogger.h>
#include <logging/cs_Logger.h>
#include <processing/cs_BackgroundAdvHandler.h>
#include <processing/cs_TapToToggle.h>
#include <storage/cs_State.h>
#include <structs/buffer/cs_EncryptionBuffer.h>
#include <time/cs_SystemTime.h>
#include <uart/cs_UartHandler.h>
#include <util/cs_Utils.h>

extern "C" {
#include <nrf_nvmc.h>
#include <util/cs_Syscalls.h>
}


/****************************************************** Preamble *******************************************************/ 

// Define test pin to enable gpio debug.
//#define CS_TEST_PIN 18

#ifdef CS_TEST_PIN
	#ifdef DEBUG
		#pragma message("Crownstone test pin enabled")
	#else
		#warning "Crownstone test pin enabled"
	#endif
#endif

cs_ram_stats_t Crownstone::_ramStats;


/****************************************** Global functions ******************************************/

/**
 * Start the 32 MHz external oscillator.
 * This provides very precise timing, which is required for the PWM driver to synchronize with the mains supply.
 * An alternative is to use the low frequency crystal oscillator for the PWM driver, but the SoftDevice's radio
 * operations periodically turn on the 32 MHz crystal oscillator anyway.
 */
void startHFClock() {
	// Reference: https://devzone.nordicsemi.com/f/nordic-q-a/6394/use-external-32mhz-crystal

	// Start the external high frequency crystal
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;

	// Wait for the external oscillator to start up
	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0) {}
	LOGd("HF clock started");
}

/**
 * If UART is enabled this will be the message printed out over a serial connection. In release mode we will not by
 * default use the UART, it will need to be turned on.
 */
void initUart(uint8_t pinRx, uint8_t pinTx) {
	serial_config(pinRx, pinTx);
	serial_init(SERIAL_ENABLE_RX_AND_TX);

	LOGi("Welcome to Bluenet!");
	LOGi(" _|_|_|    _|                                            _|     ");
	LOGi(" _|    _|  _|  _|    _|    _|_|    _|_|_|      _|_|    _|_|_|_| ");
	LOGi(" _|_|_|    _|  _|    _|  _|_|_|_|  _|    _|  _|_|_|_|    _|     ");
	LOGi(" _|    _|  _|  _|    _|  _|        _|    _|  _|          _|     ");
	LOGi(" _|_|_|    _|    _|_|_|    _|_|_|  _|    _|    _|_|_|      _|_| ");
	
	LOGi("Firmware version %s", g_FIRMWARE_VERSION);
	LOGi("Git hash %s", g_GIT_SHA1);
	LOGi("Compilation date: %s", g_COMPILATION_DAY);
	LOGi("Compilation time: %s", __TIME__);
	LOGi("Build type: %s", g_BUILD_TYPE);
	LOGi("Hardware version: %s", get_hardware_version());
	LOGi("Verbosity: %i", SERIAL_VERBOSITY);
	LOGi("UART binary protocol set: %d", CS_UART_BINARY_PROTOCOL_ENABLED);
#ifdef DEBUG
	LOGi("DEBUG: defined");
#else
	LOGi("DEBUG: undefined");
#endif
}

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
		nrf_nvmc_write_word(g_HARDWARE_BOARD_ADDRESS, g_DEFAULT_HARDWARE_BOARD);
	}
	LOGd("Board: %p", hardwareBoard);
}

/** Enable NFC pins to be used as GPIO.
 *
 * Warning: this is stored in UICR, so it's persistent.
 * Warning: NFC pins leak a bit of current when not at same voltage level.
 */
void enableNfcPins() {
	if (NRF_UICR->NFCPINS != 0) {
		nrf_nvmc_write_word((uint32_t)&(NRF_UICR->NFCPINS), 0);
	}
}

void printNfcPins() {
	uint32_t val = NRF_UICR->NFCPINS;
	if (val == 0) {
		LOGd("NFC pins enabled (%p)", val);
	}
	else {
		LOGd("NFC pins disabled (%p)", val);
	}
}

void on_exit(void) {
	LOGf("PROGRAM TERMINATED");
}


void handleZeroCrossing() {
	PWM::getInstance().onZeroCrossingInterrupt();
}

/************************************************* cs_Crownstone impl *************************************************/

Crownstone::Crownstone(boards_config_t& board) :
	_boardsConfig(board),
#if BUILD_MEM_USAGE_TEST == 1
	_memTest(board),
#endif
	_mainTimerId(NULL),
	_operationMode(OperationMode::OPERATION_MODE_UNINITIALIZED)
{
	// TODO: can be replaced by: APP_TIMER_DEF(_mainTimerId); Though that makes _mainTimerId a static variable.
	_mainTimerData = { {0} };
	_mainTimerId = &_mainTimerData;

	EncryptionBuffer::getInstance().alloc(BLE_GATTS_VAR_ATTR_LEN_MAX);

	// TODO (Anne @Arend). Yes, you can call this in constructor. All non-virtual member functions can be called as well.
	this->listen();
	_stack = &Stack::getInstance();
	_bleCentral = &BleCentral::getInstance();
	_crownstoneCentral = new CrownstoneCentral();
	_advertiser = &Advertiser::getInstance();
	_timer = &Timer::getInstance();
	_storage = &Storage::getInstance();
	_state = &State::getInstance();
	_commandHandler = &CommandHandler::getInstance();
	_factoryReset = &FactoryReset::getInstance();

	_scanner = &Scanner::getInstance();
#if BUILD_MESHING == 1
	_mesh = &Mesh::getInstance();
#endif
#if BUILD_MICROAPP_SUPPORT == 1
	_microapp = &Microapp::getInstance();
#endif

	if (IS_CROWNSTONE(_boardsConfig.deviceType)) {
		_temperatureGuard = &TemperatureGuard::getInstance();
		_powerSampler = &PowerSampling::getInstance();
	}

#if BUILD_TWI == 1
	_twi = &Twi::getInstance();
#endif

#if BUILD_GPIOTE == 1
	_gpio = &Gpio::getInstance();
#endif
};

void Crownstone::init(uint16_t step) {
	updateHeapStats();
	updateMinStackEnd();
	printLoadStats();
	switch (step) {
	case 0: {
		init0();
		break;
	}
	case 1: {
		init1();
		break;
	}
	}
}

void Crownstone::init0() {
	LOGi(FMT_HEADER, "init");
	initDrivers(0);
}

void Crownstone::init1() {
	initDrivers(1);
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

	LOGi(FMT_HEADER, "init central");
	_bleCentral->init();
	_crownstoneCentral->init();

#if BUILD_MICROAPP_SUPPORT == 1
	LOGi(FMT_HEADER, "init microapp");
	_microapp->init();
#endif
}

void Crownstone::initDrivers(uint16_t step) {
	switch (step) {
	case 0: {
		initDrivers0();
		break;
	}
	case 1: {
		initDrivers1();
		break;
	}
	}
}

void Crownstone::initDrivers0() {
	LOGi("Init drivers");
	startHFClock();
	_stack->init();
	_timer->init();
	_stack->initSoftdevice();

#if BUILD_MESHING == 1 && MESH_PERSISTENT_STORAGE == 1
	// Check if flash pages of mesh are valid, else erase them.
	// This has to be done before Storage is initialized.
	if (!_mesh->checkFlashValid()) {
		// Wait for pages erased event.
		return;
	}
#endif

	cs_ret_code_t retCode = _storage->init();
	if (retCode != ERR_SUCCESS) {
		// We can try to erase all pages.
		retCode = _storage->eraseAllPages();
		if (retCode != ERR_SUCCESS) {
			// Only option left is to reboot and see if things work out next time.
			APP_ERROR_CHECK(NRF_ERROR_INVALID_STATE);
		}
		// Wait for pages erased event.
		return;
	}
	// Wait for storage initialized event.
}

void Crownstone::initDrivers1() {
	_state->init(&_boardsConfig);

	// If not done already, init UART
	// TODO: make into a class with proper init() function
	if (!_boardsConfig.flags.hasSerial) {
		serial_config(_boardsConfig.pinGpioRx, _boardsConfig.pinGpioTx);
		TYPIFY(CONFIG_UART_ENABLED) uartEnabled;
		_state->get(CS_TYPE::CONFIG_UART_ENABLED, &uartEnabled, sizeof(uartEnabled));
		serial_enable((serial_enable_t)uartEnabled);
		UartHandler::getInstance().init((serial_enable_t)uartEnabled);
	}
	else {
		// Init UartHandler only now, because it will read State.
		UartHandler::getInstance().init(SERIAL_ENABLE_RX_AND_TX);
	}

	// Plain text log.
	CLOGi("\r\nFirmware version %s", g_FIRMWARE_VERSION);

	LOGi("GPRegRet: %u %u", GpRegRet::getValue(GpRegRet::GPREGRET), GpRegRet::getValue(GpRegRet::GPREGRET2));

	// Store reset reason.
	sd_power_reset_reason_get(&_resetReason);
	LOGi("Reset reason: %u - watchdog=%u soft=%u lockup=%u off=%u", _resetReason,
			(_resetReason & NRF_POWER_RESETREAS_DOG_MASK) != 0,
			(_resetReason & NRF_POWER_RESETREAS_SREQ_MASK) != 0,
			(_resetReason & NRF_POWER_RESETREAS_LOCKUP_MASK) != 0,
			(_resetReason & NRF_POWER_RESETREAS_OFF_MASK) != 0);

	// Store gpregret.
	_gpregret[0] = GpRegRet::getValue(GpRegRet::GPREGRET);
	_gpregret[1] = GpRegRet::getValue(GpRegRet::GPREGRET2);

	if (GpRegRet::isFlagSet(GpRegRet::FLAG_STORAGE_RECOVERED)) {
		_setStateValuesAfterStorageRecover = true;
		GpRegRet::clearFlag(GpRegRet::FLAG_STORAGE_RECOVERED);
	}
	if (_setStateValuesAfterStorageRecover) {
		LOGw("Set state values after storage recover.");
		// Set switch state to on, as that's the most likely and preferred state of the switch.
		TYPIFY(STATE_SWITCH_STATE) switchState;
		switchState.state.dimmer = 0;
		switchState.state.relay = 1;
		_state->set(CS_TYPE::STATE_SWITCH_STATE, &switchState, sizeof(switchState));
	}

	LOGi(FMT_INIT, "command handler");
	_commandHandler->init(&_boardsConfig);

	LOGi(FMT_INIT, "factory reset");
	_factoryReset->init();

	LOGi(FMT_INIT, "encryption");
	ConnectionEncryption::getInstance().init();
	KeysAndAccess::getInstance().init();


	if (IS_CROWNSTONE(_boardsConfig.deviceType)) {
		LOGi(FMT_INIT, "switch");
		_switchAggregator.init(_boardsConfig);

		LOGi(FMT_INIT, "temperature guard");
		_temperatureGuard->init(_boardsConfig);

		LOGi(FMT_INIT, "power sampler");
		_powerSampler->init(_boardsConfig);
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

#if BUILD_TWI == 1
	_twi->init(_boardsConfig);
#endif

#if BUILD_GPIOTE == 1
	_gpio->init(_boardsConfig);
#endif
}

void Crownstone::configure() {
	assert(_stack != NULL, "Stack");
	assert(_storage != NULL, "Storage");

	LOGi("> stack ...");
	_stack->initRadio();

	// Don't do garbage collection now, it will block reading flash.
//	_storage->garbageCollect();

	increaseResetCounter();

	setName(true);

	LOGi("> advertisement ...");
	configureAdvertisement();
}

void Crownstone::configureAdvertisement() {

	// Set the stored advertisement interval
	TYPIFY(CONFIG_ADV_INTERVAL) advInterval;
	_state->get(CS_TYPE::CONFIG_ADV_INTERVAL, &advInterval, sizeof(advInterval));
	_advertiser->setAdvertisingInterval(advInterval);
	_advertiser->init();

	// Create the ServiceData object which will be used to advertise select state variables from the Crownstone.
	_serviceData = new ServiceData();
	_serviceData->init(_boardsConfig.deviceType);

	// Assign service data to stack
	_advertiser->setServiceData(_serviceData);
	_advertiser->configureAdvertisement(_boardsConfig.deviceType);
}

void Crownstone::createService(const ServiceEvent event) {
	switch (event) {
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

void Crownstone::switchMode(const OperationMode & newMode) {

	LOGd("Current mode: %s", operationModeName(_oldOperationMode));
	LOGd("Switch to mode: %s", operationModeName(newMode));

	switch (_oldOperationMode) {
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

//	_stack->halt();

	// Remove services that belong to the current operation mode.
	// This is not done... It is impossible to remove services in the SoftDevice.

	// Start operation mode
	startOperationMode(newMode);

	// Create services that belong to the new mode.
	switch (newMode) {
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

//	_stack->resume();

	switch (newMode) {
		case OperationMode::OPERATION_MODE_SETUP: {
			LOGd("Configure setup mode");
//			_advertiser->changeToLowTxPower();
			_advertiser->changeToNormalTxPower();
			break;
		}
		case OperationMode::OPERATION_MODE_FACTORY_RESET: {
			LOGd("Configure factory reset mode");
#if BUILD_MESHING == 1 && MESH_PERSISTENT_STORAGE == 1
			// Seems like mesh has to be started in order to run the factory reset.
			_mesh->init(_boardsConfig);
			_mesh->start();
#endif
			FactoryReset::getInstance().finishFactoryReset(_boardsConfig.deviceType);
			_advertiser->changeToNormalTxPower();
			break;
		}
		case OperationMode::OPERATION_MODE_DFU: {
			LOGd("Configure DFU mode");
			// TODO: have this function somewhere else.
			cs_result_t result;
			CommandHandler::getInstance().handleCommand(CS_CONNECTION_PROTOCOL_VERSION, CTRL_CMD_GOTO_DFU, cs_data_t(), cmd_source_with_counter_t(CS_CMD_SOURCE_INTERNAL), ADMIN, result);
			_advertiser->changeToNormalTxPower();
			break;
		}
		default:
			_advertiser->changeToNormalTxPower();
	}

	// Enable AES encryption.
	if (_state->isTrue(CS_TYPE::CONFIG_ENCRYPTION_ENABLED)) {
		LOGi(FMT_ENABLE, "AES encryption");
		_stack->setAesEncrypted(true);
	}

//	_operationMode = newMode;
}

void Crownstone::setName(bool firstTime) {
	char device_name[32];
	cs_state_data_t stateNameData(CS_TYPE::CONFIG_NAME, (uint8_t*)device_name, sizeof(device_name));
	_state->get(stateNameData);
	std::string deviceName;
	if (g_CHANGE_NAME_ON_RESET) {
		//! clip name to 5 chars and add reset counter at the end
		TYPIFY(STATE_RESET_COUNTER) resetCounter;
		_state->get(CS_TYPE::STATE_RESET_COUNTER, &resetCounter, sizeof(resetCounter));
		char devicename_resetCounter[32];
		sprintf(devicename_resetCounter, "%.*s_%d", MIN(stateNameData.size, 5), device_name, resetCounter);
		deviceName = std::string(devicename_resetCounter);
	} else {
		deviceName = std::string(device_name, MIN(stateNameData.size, 5));
	}
	if (firstTime) {
		_advertiser->setDeviceName(deviceName);
	} else {
		_advertiser->updateDeviceName(deviceName);
	}
}

void Crownstone::startOperationMode(const OperationMode & mode) {
	_behaviourStore.listen();
	_presenceHandler.listen();

	switch (mode) {
		case OperationMode::OPERATION_MODE_NORMAL: {
			_scanner->init();
			_scanner->setStack(_stack);

#if BUILD_MESHING == 1
			if (_state->isTrue(CS_TYPE::CONFIG_MESH_ENABLED)) {
				_mesh->init(_boardsConfig);
			}
#endif
			RC5::getInstance().init();

			_commandAdvHandler = &CommandAdvHandler::getInstance();
			_commandAdvHandler->init();

			BackgroundAdvertisementHandler::getInstance();

			_multiSwitchHandler = &MultiSwitchHandler::getInstance();
			_multiSwitchHandler->init();

			_meshTopology.init();

			_assetFiltering.init();

			break;
		}
		case OperationMode::OPERATION_MODE_SETUP: {
			// TODO: Why this hack?
			if (serial_get_state() == SERIAL_ENABLE_NONE) {
				serial_enable(SERIAL_ENABLE_RX_ONLY);
				UartHandler::getInstance().init(SERIAL_ENABLE_RX_ONLY);
			}
			break;
		}
		default: {
			// nothing to be done
			break;
		}
	}
}

void Crownstone::startUp() {

	LOGi(FMT_HEADER, "startup");

	TYPIFY(CONFIG_BOOT_DELAY) bootDelay;
	_state->get(CS_TYPE::CONFIG_BOOT_DELAY, &bootDelay, sizeof(bootDelay));
	if (bootDelay) {
		LOGi("Boot delay: %d ms", bootDelay);
		nrf_delay_ms(bootDelay);
	}

	//! start advertising
	LOGi("Start advertising");
	_advertiser->startAdvertising();

	// Set the stored tx power, must be done after advertising has started.
	TYPIFY(CONFIG_TX_POWER) txPower = 0;
	_state->get(CS_TYPE::CONFIG_TX_POWER, &txPower, sizeof(txPower));
	_advertiser->updateTxPower(txPower);

	// Have to give the stack a moment of pause to start advertising, otherwise we get into race conditions.
	// TODO: Is this still the case? Can we solve this differently?
	nrf_delay_ms(50);

	if (IS_CROWNSTONE(_boardsConfig.deviceType)) {
		_switchAggregator.switchPowered();

		//! Start temperature guard regardless of operation mode
		LOGi(FMT_START, "temp guard");
		_temperatureGuard->start();

		//! Start power sampler regardless of operation mode (as it is used for the current based soft fuse)
		LOGi(FMT_START, "power sampling");
		_powerSampler->startSampling();

		// Let the power sampler call the PWM callback function on zero crossings.
		_powerSampler->enableZeroCrossingInterrupt(handleZeroCrossing);
	}

	// Start ticking main and services.
	scheduleNextTick();
	SystemTime::init();

	// The rest we only execute if we are in normal operation.
	// During other operation modes, most of the crownstone's functionality is disabled.
	if (_operationMode == OperationMode::OPERATION_MODE_NORMAL) {
		_systemTime.listen();
		
		TapToToggle::getInstance().init(_boardsConfig.tapToToggleDefaultRssiThreshold);

		_trackedDevices.init();

		if (_state->isTrue(CS_TYPE::CONFIG_SCANNER_ENABLED)) {
			uint16_t delay = RNG::getInstance().getRandom16() / 6; // Delay in ms (about 0-10 seconds)
			_scanner->delayedStart(delay);
		}

		if (_state->isTrue(CS_TYPE::CONFIG_MESH_ENABLED)) {
#if BUILD_MESHING == 1
			_mesh->start();
			if (_state->isTrue(CS_TYPE::CONFIG_IBEACON_ENABLED)) {
				_mesh->initAdvertiser();
				_mesh->advertiseIbeacon();
			}
#endif
		}
		else {
			LOGi("Mesh not enabled");
		}

		_behaviourStore.init();
	}

	uint32_t err_code;
	ble_gap_addr_t address;
	err_code = sd_ble_gap_addr_get(&address);
	APP_ERROR_CHECK(err_code);

	_log(SERIAL_INFO, false, "Address: ");
	BLEutil::printAddress((uint8_t*)address.addr, BLE_GAP_ADDR_LEN, SERIAL_INFO);
	LOGi("Address id=%u type=%u", address.addr_id_peer, address.addr_type);

	// Plain text log.
	CLOGi("\r\nAddress: %X:%X:%X:%X:%X:%X", address.addr[5], address.addr[4], address.addr[3], address.addr[2], address.addr[1], address.addr[0]);

	_state->startWritesToFlash();

#if BUILD_MESHING == 1
	_mesh->startSync();
#endif

#if BUILD_MEM_USAGE_TEST == 1
	if (_operationMode == OperationMode::OPERATION_MODE_NORMAL) {
		_memTest.start();
	}
#endif

	// Clear all reset reasons after initializing and starting all modules.
	// So the modules had the opportunity to read it out.
	sd_power_reset_reason_clr(0xFFFFFFFF);
}

void Crownstone::increaseResetCounter() {
	TYPIFY(STATE_RESET_COUNTER) resetCounter = 0;
	_state->get(CS_TYPE::STATE_RESET_COUNTER, &resetCounter, sizeof(resetCounter));
	resetCounter++;
	LOGi("Reset counter at %u", resetCounter);
	_state->set(CS_TYPE::STATE_RESET_COUNTER, &resetCounter, sizeof(resetCounter));
}

void Crownstone::tick() {
	updateHeapStats();
	if (_tickCount % (60*1000/TICK_INTERVAL_MS) == 0) {
		printLoadStats();
	}

	if (_tickCount % (500/TICK_INTERVAL_MS) == 0) {
		TYPIFY(STATE_TEMPERATURE) temperature = getTemperature();
		_state->set(CS_TYPE::STATE_TEMPERATURE, &temperature, sizeof(temperature));
	}

	if (!_clearedGpRegRetCount && _tickCount == (CS_CLEAR_GPREGRET_COUNTER_TIMEOUT_S * 1000 / TICK_INTERVAL_MS)) {
		GpRegRet::clearAll();
		_clearedGpRegRetCount = true;
	}

#if BUILD_MEM_USAGE_TEST == 1
	_memTest.onTick();
#endif

	Watchdog::kick();

	event_t event(CS_TYPE::EVT_TICK, &_tickCount, sizeof(_tickCount));
	event.dispatch();
	++_tickCount;

	scheduleNextTick();
}

void Crownstone::scheduleNextTick() {
	Timer::getInstance().start(_mainTimerId, MS_TO_TICKS(TICK_INTERVAL_MS), this);
}

void Crownstone::run() {
	LOGi(FMT_HEADER, "running");

	while (1) {
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

void Crownstone::handleEvent(event_t & event) {

	switch (event.type) {
		case CS_TYPE::EVT_STORAGE_INITIALIZED:
			init(1);
			startUp();
			LOG_FLUSH();
			break;
		case CS_TYPE::EVT_STORAGE_PAGES_ERASED: {
			LOGi("Storage pages erased, reboot");
			/*
			 * FDS init can't be called a second time, because it remembers that it's initializing, though
			 * doesn't continue doing so.
			 *
			 * The following commented out code could be used once FDS has been fixed.
			 * For now, we use GPREGRET to remember and do it next boot.
			 */
//			cs_ret_code_t retCode = _storage->init();
//			if (retCode != ERR_SUCCESS) {
//				LOGf("Storage init failed after page erase");
//				// Only option left is to reboot and see if things work out next time.
//				APP_ERROR_CHECK(NRF_ERROR_INTERNAL);
//			}
//
//			_setStateValuesAfterStorageRecover = true;
//			// Wait for storage initialized event.
			GpRegRet::setFlag(GpRegRet::FLAG_STORAGE_RECOVERED);
			sd_nvic_SystemReset();
			break;
		}
		case CS_TYPE::EVT_MESH_PAGES_ERASED: {
			LOGi("Mesh pages erased, reboot");
			sd_nvic_SystemReset();
			break;
		}
		case CS_TYPE::CMD_GET_SCHEDULER_MIN_FREE: {
			LOGi("Get scheduler min free");
			uint16_t minFree;
			if (event.result.buf.len < sizeof(minFree)) {
				event.result.returnCode = ERR_BUFFER_TOO_SMALL;
				break;
			}
			minFree = SCHED_QUEUE_SIZE - app_sched_queue_utilization_get();
			memcpy(event.result.buf.data, &minFree, sizeof(minFree));
			event.result.dataSize = sizeof(minFree);
			event.result.returnCode = ERR_SUCCESS;
			break;
		}
		case CS_TYPE::CMD_GET_RESET_REASON: {
			LOGi("Get reset reason");
			if (event.result.buf.len < sizeof(_resetReason)) {
				event.result.returnCode = ERR_BUFFER_TOO_SMALL;
				break;
			}
			memcpy(event.result.buf.data, &_resetReason, sizeof(_resetReason));
			event.result.dataSize = sizeof(_resetReason);
			event.result.returnCode = ERR_SUCCESS;
			break;
		}
		case CS_TYPE::CMD_GET_GPREGRET: {
			LOGi("Get GPREGRET");
			cs_gpregret_result_t gpregret;
			if (event.result.buf.len < sizeof(gpregret)) {
				event.result.returnCode = ERR_BUFFER_TOO_SMALL;
				break;
			}
			TYPIFY(CMD_GET_GPREGRET) index = *((TYPIFY(CMD_GET_GPREGRET)*)event.data);
			if (index > sizeof(_gpregret) / sizeof(_gpregret[0])) {
				event.result.returnCode = ERR_WRONG_PARAMETER;
				break;
			}
			gpregret.index = index;
			gpregret.value = _gpregret[index];
			memcpy(event.result.buf.data, &(gpregret), sizeof(gpregret));
			event.result.dataSize = sizeof(gpregret);
			event.result.returnCode = ERR_SUCCESS;
			break;
		}
		case CS_TYPE::CMD_GET_RAM_STATS: {
			LOGi("Get RAM stats");
			if (event.result.buf.len < sizeof(_ramStats)) {
				event.result.returnCode = ERR_BUFFER_TOO_SMALL;
				break;
			}
			memcpy(event.result.buf.data, &_ramStats, sizeof(_ramStats));
			event.result.dataSize = sizeof(_ramStats);
			event.result.returnCode = ERR_SUCCESS;
			break;
		}
		default:
			LOGnone("Event: $typeName(%u)", to_underlying_type(event.type));
	}

	// 	case CS_TYPE::CONFIG_IBEACON_ENABLED: {
	// 		__attribute__((unused)) TYPIFY(CONFIG_IBEACON_ENABLED) enabled = *(TYPIFY(CONFIG_IBEACON_ENABLED)*)event.data;
	// 		// 12-sep-2019 TODO: implement
	// 		LOGw("TODO ibeacon enabled=%i", enabled);
	// 		break;
	// 	}

}

void Crownstone::updateHeapStats() {
	// Don't have to do much, _sbrk() is the best place to keep up the heap end.
	_ramStats.maxHeapEnd = (uint32_t)getHeapEndMax();
	_ramStats.minFree = _ramStats.minStackEnd - _ramStats.maxHeapEnd;
	_ramStats.numSbrkFails = getSbrkNumFails();
}

void Crownstone::updateMinStackEnd() {
	void* stackPointer;
	asm("mov %0, sp" : "=r"(stackPointer) : : );
	if ((uint32_t)stackPointer < _ramStats.minStackEnd) {
		_ramStats.minStackEnd = (uint32_t)stackPointer;
	}
}

void Crownstone::printLoadStats() {
	// Log ram usage.
	uint8_t *heapPointer = (uint8_t*)malloc(1);
	void* stackPointer;
	asm("mov %0, sp" : "=r"(stackPointer) : : );
	LOGd("Memory heap=%p, stack=%p", heapPointer, (uint8_t*)stackPointer);
	free(heapPointer);

	LOGi("heapEnd=0x%X maxHeapEnd=0x%X minStackEnd=0x%X minFree=%u sbrkFails=%u", (uint32_t)getHeapEnd(), _ramStats.maxHeapEnd, _ramStats.minStackEnd, _ramStats.minFree, _ramStats.numSbrkFails);

	// Log scheduler usage.
	__attribute__((unused)) uint16_t maxUsed = app_sched_queue_utilization_get();
	__attribute__((unused)) uint16_t currentFree = app_sched_queue_space_get();
	LOGi("Scheduler current free=%u max used=%u", currentFree, maxUsed);
}

void printBootloaderInfo() {
	bluenet_ipc_bootloader_data_t bootloaderData;
	uint8_t size = sizeof(bootloaderData);
	uint8_t dataSize;
	uint8_t *buf = (uint8_t*)&bootloaderData;
	int retCode = getRamData(IPC_INDEX_BOOTLOADER_VERSION, buf, size, &dataSize);
	if (retCode != IPC_RET_SUCCESS) {
		LOGw("No IPC data found, error = %i", retCode);
		return;
	}
	if (size != dataSize) {
		LOGw("IPC data struct incorrect size");
		return;
	}
	LOGd("Bootloader version protocol=%u dfu_version=%u build_type=%u",
			bootloaderData.protocol,
			bootloaderData.dfu_version,
			bootloaderData.build_type);
	LOGi("Bootloader version: %u.%u.%u-RC%u",
			bootloaderData.major,
			bootloaderData.minor,
			bootloaderData.patch,
			bootloaderData.prerelease);
}

/**********************************************************************************************************************
 * The main function. Note that this is not the first function called! For starters, if there is a bootloader present,
 * the code within the bootloader has been processed before. But also after the bootloader, the code in
 * cs_sysNrf51.c will set event handlers and other stuff (such as coping with product anomalies, PAN), before calling
 * main. If you enable a new peripheral device, make sure you enable the corresponding event handler there as well.
 *********************************************************************************************************************/

int main() {
#ifdef CS_TEST_PIN
	nrf_gpio_cfg_output(CS_TEST_PIN);
	nrf_gpio_pin_clear(CS_TEST_PIN);
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

	// Init GPIO pins early in the process!
	switch (board.hardwareBoard) {
		// These boards use the NFC pins (p0.09 and p0.10).
		// They have to be configured as GPIO before they can be used as GPIO.
		case ACR01B10D:
			enableNfcPins();
			break;
		default:
			break;
	}
	if (IS_CROWNSTONE(board.deviceType)) {
		nrf_gpio_cfg_output(board.pinGpioPwm);
		if (board.flags.pwmInverted) {
			nrf_gpio_pin_set(board.pinGpioPwm);
		}
		else {
			nrf_gpio_pin_clear(board.pinGpioPwm);
		}
		nrf_gpio_cfg_output(board.pinGpioEnablePwm);
		nrf_gpio_pin_clear(board.pinGpioEnablePwm);
		//! Relay pins
		if (board.flags.hasRelay) {
			nrf_gpio_cfg_output(board.pinGpioRelayOff);
			nrf_gpio_pin_clear(board.pinGpioRelayOff);
			nrf_gpio_cfg_output(board.pinGpioRelayOn);
			nrf_gpio_pin_clear(board.pinGpioRelayOn);
		}
	}

	if (board.flags.hasSerial) {
		initUart(board.pinGpioRx, board.pinGpioTx);
		LOG_FLUSH();
	}

	// Start the watchdog early (after uart, so we can see the logs).
	Watchdog::init();
	Watchdog::start();

	printNfcPins();
	LOG_FLUSH();

	printBootloaderInfo();


//	// Make a "clicker"
//	nrf_delay_ms(1000);
//	nrf_gpio_pin_set(board.pinGpioRelayOn);
//	nrf_delay_ms(RELAY_HIGH_DURATION);
//	nrf_gpio_pin_clear(board.pinGpioRelayOn);
//	while (true) {
//		nrf_delay_ms(1 * 60 * 1000); // 1 minute on
//		nrf_gpio_pin_set(board.pinGpioRelayOff);
//		nrf_delay_ms(RELAY_HIGH_DURATION);
//		nrf_gpio_pin_clear(board.pinGpioRelayOff);
//		nrf_delay_ms(5 * 60 * 1000); // 5 minutes off
//		nrf_gpio_pin_set(board.pinGpioRelayOn);
//		nrf_delay_ms(RELAY_HIGH_DURATION);
//		nrf_gpio_pin_clear(board.pinGpioRelayOn);
//	}

	Crownstone crownstone(board);

	overwrite_hardware_version();

	// init drivers, configure(), create services and chars,
	crownstone.init(0);
	LOG_FLUSH();

	// run forever ...
	crownstone.run();

	return 0;
}
