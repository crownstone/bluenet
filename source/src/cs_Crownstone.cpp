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
#include <drivers/cs_Uicr.h>
#include <drivers/cs_Watchdog.h>
#include <encryption/cs_ConnectionEncryption.h>
#include <encryption/cs_RC5.h>
#include <ipc/cs_IpcRamData.h>
#include <logging/cs_CLogger.h>
#include <logging/cs_Logger.h>
#include <processing/cs_BackgroundAdvHandler.h>
#include <processing/cs_TapToToggle.h>
#include <storage/cs_State.h>
#include <structs/buffer/cs_CharacteristicReadBuffer.h>
#include <structs/buffer/cs_CharacteristicWriteBuffer.h>
#include <structs/buffer/cs_EncryptedBuffer.h>
#include <time/cs_SystemTime.h>
#include <uart/cs_UartHandler.h>
#include <util/cs_Utils.h>

extern "C" {
#include <util/cs_Syscalls.h>
}

#define LOGCrownstoneDebug LOGd

/****************************************************** Preamble
 * *******************************************************/

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
	NRF_CLOCK->TASKS_HFCLKSTART    = 1;

	// Wait for the external oscillator to start up
	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0) {
	}
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

	// Plain text log for when there are difficulties with the binary log parser.
	// This means that when a device is returned to us (and binary logging is on) we can derive which firmware
	// is present on it without having to parse the data coming from the device.
#if CS_UART_BINARY_PROTOCOL_ENABLED == 1
	CLOGi("\r\nFirmware version %s", g_FIRMWARE_VERSION);
	CLOGi("\r\nGit hash %s", g_GIT_SHA1);
#endif

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
	cs_ret_code_t retCode = writeHardwareBoard();
	LOGd("Board: %p", getHardwareBoard());
	if (retCode != ERR_SUCCESS) {
		LOGe("Failed to write hardware board: retCode=%u", retCode);
	}
}

void printNfcPins() {
	LOGd("NFC pins used as gpio: %u", canUseNfcPinsAsGpio());
}

void on_exit(void) {
	LOGf("PROGRAM TERMINATED");
}

void handleZeroCrossing() {
	PWM::getInstance().onZeroCrossingInterrupt();
}

/************************************************* cs_Crownstone impl *************************************************/

Crownstone::Crownstone(boards_config_t& board)
		: _boardsConfig(board)
		,
#if BUILD_MEM_USAGE_TEST == 1
		_memTest(board)
		,
#endif
		_mainTimerId(NULL)
		, _operationMode(OperationMode::OPERATION_MODE_UNINITIALIZED) {
	// TODO: can be replaced by: APP_TIMER_DEF(_mainTimerId); Though that makes _mainTimerId a static variable.
	_mainTimerData = {{0}};
	_mainTimerId   = &_mainTimerData;

	// TODO (Anne @Arend). Yes, you can call this in constructor. All non-virtual member functions can be called as
	// well.
	this->listen();
	_stack             = &Stack::getInstance();
	_bleCentral        = &BleCentral::getInstance();
	_crownstoneCentral = new CrownstoneCentral();
	_advertiser        = &Advertiser::getInstance();
	_timer             = &Timer::getInstance();
	_storage           = &Storage::getInstance();
	_state             = &State::getInstance();
	_commandHandler    = &CommandHandler::getInstance();
	_factoryReset      = &FactoryReset::getInstance();

	_scanner           = &Scanner::getInstance();
#if BUILD_MESHING == 1
	_mesh = &Mesh::getInstance();
#endif
#if BUILD_MICROAPP_SUPPORT == 1
	_microapp = &Microapp::getInstance();
#endif

	if (IS_CROWNSTONE(_boardsConfig.deviceType)) {
		_temperatureGuard = &TemperatureGuard::getInstance();
		_powerSampler     = &PowerSampling::getInstance();
	}

#if BUILD_TWI == 1
	_twi = &Twi::getInstance();
#endif

#if BUILD_GPIOTE == 1
	_gpio = &Gpio::getInstance();
#endif

	parentAllChildren();
};

std::vector<Component*> Crownstone::getChildren() {
	return {
			&_switchAggregator, &_presenceHandler, &_behaviourStore,
			// TODO: add others (when necessary for weak dependences)
	};
}

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
	LOGi(FMT_HEADER "init");
	initDrivers0();
}

void Crownstone::init1() {
	initDrivers1();
	LOG_FLUSH();

	TYPIFY(STATE_OPERATION_MODE) mode;
	_state->get(CS_TYPE::STATE_OPERATION_MODE, &mode, sizeof(mode));
	LOGCrownstoneDebug("Persisted operation mode is 0x%X", mode);

	_operationMode = getOperationMode(mode);
	LOGCrownstoneDebug("Resolved operation mode is 0x%X", _operationMode);

	//! configure the crownstone
	LOGi(FMT_HEADER "configure");
	configure();
	LOG_FLUSH();

	LOGi(FMT_CREATE "timer");
	_timer->createSingleShot(_mainTimerId, (app_timer_timeout_handler_t)Crownstone::staticTick);
	LOG_FLUSH();

	LOGi(FMT_HEADER "mode");
	switchMode(_operationMode);
	LOG_FLUSH();

	LOGi(FMT_HEADER "init services");
	_stack->initServices();
	LOG_FLUSH();

	LOGi(FMT_HEADER "init central");
	_bleCentral->init();
	_crownstoneCentral->init();

#if BUILD_MICROAPP_SUPPORT == 1
	LOGi(FMT_HEADER "init microapp");
	_microapp->init();
#endif
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
	if (!_boardsConfig.flags.enableUart) {
		serial_config(_boardsConfig.pinRx, _boardsConfig.pinTx);
		TYPIFY(CONFIG_UART_ENABLED) uartEnabled;
		_state->get(CS_TYPE::CONFIG_UART_ENABLED, &uartEnabled, sizeof(uartEnabled));
		serial_init(static_cast<serial_enable_t>(uartEnabled));
		UartHandler::getInstance().init((serial_enable_t)uartEnabled);
	}
	else {
		// Init UartHandler only now, because it will read State.
		UartHandler::getInstance().init(SERIAL_ENABLE_RX_AND_TX);
	}

	LOGi("GPRegRet: %u %u", GpRegRet::getValue(GpRegRet::GPREGRET), GpRegRet::getValue(GpRegRet::GPREGRET2));

	// Store reset reason.
	sd_power_reset_reason_get(&_resetReason);
	LOGi("Reset reason: %u - watchdog=%u soft=%u lockup=%u off=%u",
		 _resetReason,
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
		switchState.state.relay  = 1;
		_state->set(CS_TYPE::STATE_SWITCH_STATE, &switchState, sizeof(switchState));
	}

	LOGi(FMT_INIT "command handler");
	_commandHandler->init(&_boardsConfig);

	LOGi(FMT_INIT "factory reset");
	_factoryReset->init();

	LOGi(FMT_INIT "encryption");
	ConnectionEncryption::getInstance().init();
	KeysAndAccess::getInstance().init();

	if (IS_CROWNSTONE(_boardsConfig.deviceType)) {
		LOGi(FMT_INIT "switch");
		_switchAggregator.init(_boardsConfig);

		LOGi(FMT_INIT "temperature guard");
		_temperatureGuard->init(_boardsConfig);

		LOGi(FMT_INIT "power sampler");
		_powerSampler->init(&_boardsConfig);
	}

	// init GPIOs
	if (_boardsConfig.flags.enableLeds) {
		LOGi("Configure LEDs");

		for (int i = 0; i < LED_COUNT; ++i) {
			if (_boardsConfig.pinLed[i] == PIN_NONE) {
				continue;
			}
			nrf_gpio_cfg_output(_boardsConfig.pinLed[i]);

			// Turn the leds off
			uint32_t value = _boardsConfig.flags.ledInverted ? 1 : 0;
			nrf_gpio_pin_write(_boardsConfig.pinLed[i], value);
		}
	}

#if BUILD_TWI == 1
	_twi->init(_boardsConfig);
#else
	LOGi("Init: TWI module NOT enabled");
#endif

#if BUILD_GPIOTE == 1
	_gpio->init(_boardsConfig);
#else
	LOGi("Init: Gpio module NOT enabled");
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

	LOGi("> advertisement ...");
	configureAdvertisement();
}

void Crownstone::configureAdvertisement() {
	setName();

	// Set the stored advertisement interval
	TYPIFY(CONFIG_ADV_INTERVAL) advInterval;
	_state->get(CS_TYPE::CONFIG_ADV_INTERVAL, &advInterval, sizeof(advInterval));
	_advertiser->setAdvertisingInterval(advInterval);
	_advertiser->init();

	// Create the ServiceData object which will be used to advertise select state variables from the Crownstone.
	_serviceData = new ServiceData();
	_serviceData->init(_boardsConfig.deviceType);

	// Set the service data as advertisement data.
	_advertiser->configureAdvertisement(*_serviceData, false);
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
		default: LOGe("Unknown creation event");
	}
}

void Crownstone::switchMode(const OperationMode& newMode) {

	LOGd("Current mode: %s", operationModeName(_oldOperationMode));
	LOGd("Switch to mode: %s", operationModeName(newMode));

	switch (_oldOperationMode) {
		case OperationMode::OPERATION_MODE_UNINITIALIZED: break;
		case OperationMode::OPERATION_MODE_DFU:
		case OperationMode::OPERATION_MODE_NORMAL:
		case OperationMode::OPERATION_MODE_SETUP:
		case OperationMode::OPERATION_MODE_FACTORY_RESET:
			LOGe("Only switching from UNINITIALIZED to another mode is supported");
			break;
		default: LOGe("Unknown mode %i!", newMode); return;
	}

	//	_stack->halt();

	// Remove services that belong to the current operation mode.
	// This is not done... It is impossible to remove services in the SoftDevice.

	// Start operation mode
	startOperationMode(newMode);

	// Init buffers, used by characteristics and central.
	CharacteristicReadBuffer::getInstance().alloc(g_MASTER_BUFFER_SIZE);
	CharacteristicWriteBuffer::getInstance().alloc(g_MASTER_BUFFER_SIZE);
	// The encrypted buffer needs some extra due to overhead: header and padding.
	uint16_t encryptedSize = ConnectionEncryption::getInstance().getEncryptedBufferSize(
			g_MASTER_BUFFER_SIZE, ConnectionEncryptionType::CTR);
	EncryptedBuffer::getInstance().alloc(encryptedSize);

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

	switch (newMode) {
		case OperationMode::OPERATION_MODE_SETUP: {
			LOGd("Configure setup mode");
			//			_advertiser->changeToLowTxPower();
			_advertiser->setNormalTxPower();
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
			_advertiser->setNormalTxPower();
			break;
		}
		case OperationMode::OPERATION_MODE_DFU: {
			LOGd("Configure DFU mode");
			// TODO: have this function somewhere else.
			cs_result_t result;
			CommandHandler::getInstance().handleCommand(
					CS_CONNECTION_PROTOCOL_VERSION,
					CTRL_CMD_GOTO_DFU,
					cs_data_t(),
					cmd_source_with_counter_t(CS_CMD_SOURCE_INTERNAL),
					ADMIN,
					result);
			_advertiser->setNormalTxPower();
			break;
		}
		default: _advertiser->setNormalTxPower();
	}

	//	_operationMode = newMode;
}

void Crownstone::setName() {
	char stateName[TypeSize(CS_TYPE::CONFIG_NAME)];
	cs_state_data_t stateNameData(CS_TYPE::CONFIG_NAME, (uint8_t*)stateName, sizeof(stateName));
	_state->get(stateNameData);
	std::string name;
	if (g_CHANGE_NAME_ON_RESET) {
		// clip name and add reset counter at the end.
		TYPIFY(STATE_RESET_COUNTER) resetCounter;
		_state->get(CS_TYPE::STATE_RESET_COUNTER, &resetCounter, sizeof(resetCounter));
		char nameResetCounter[TypeSize(CS_TYPE::CONFIG_NAME)];
		sprintf(nameResetCounter, "%.*s%u", MIN(stateNameData.size, 2), stateName, resetCounter);
		name = std::string(nameResetCounter);
	}
	else {
		name = std::string(stateName, stateNameData.size);
	}
	_advertiser->setDeviceName(name);
}

void Crownstone::startOperationMode(const OperationMode& mode) {
	// TODO: should only be needed in normal mode.
	_behaviourStore.listen();
	_presenceHandler.init();

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
			// In setup mode, we want to listen for incoming UART commands,
			// so that the programmer can use UART to test it.
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

	LOGi(FMT_HEADER "startup");

	TYPIFY(CONFIG_BOOT_DELAY) bootDelay;
	_state->get(CS_TYPE::CONFIG_BOOT_DELAY, &bootDelay, sizeof(bootDelay));
	if (bootDelay) {
		LOGi("Boot delay: %d ms", bootDelay);
		nrf_delay_ms(bootDelay);
	}

	LOGi("Start advertising");
	_advertiser->startAdvertising();

	// Have to give the stack a moment of pause to start advertising, otherwise we get into race conditions.
	// TODO: Is this still the case? Can we solve this differently?
	nrf_delay_ms(50);

	if (IS_CROWNSTONE(_boardsConfig.deviceType)) {
		_switchAggregator.switchPowered();

		//! Start temperature guard regardless of operation mode
		LOGi(FMT_START "temp guard");
		_temperatureGuard->start();

		//! Start power sampler regardless of operation mode (as it is used for the current based soft fuse)
		LOGi(FMT_START "power sampling");
		_powerSampler->startSampling();

		// Let the power sampler call the PWM callback function on zero crossings.
		_powerSampler->enableZeroCrossingInterrupt(handleZeroCrossing);
	}

	// Start ticking main and services.
	scheduleNextTick();
	_systemTime.init();

	// The rest we only execute if we are in normal operation.
	// During other operation modes, most of the crownstone's functionality is disabled.
	if (_operationMode == OperationMode::OPERATION_MODE_NORMAL) {
		_systemTime.listen();

		TapToToggle::getInstance().init(_boardsConfig.tapToToggleDefaultRssiThreshold);

		_trackedDevices.init();

		if (_state->isTrue(CS_TYPE::CONFIG_SCANNER_ENABLED)) {
			uint16_t delay = RNG::getInstance().getRandom16() / 6;  // Delay in ms (about 0-10 seconds)
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
	CsUtils::printAddress((uint8_t*)address.addr, BLE_GAP_ADDR_LEN, SERIAL_INFO);
	LOGi("Address id=%u type=%u", address.addr_id_peer, address.addr_type);

	_state->startWritesToFlash();

#if BUILD_MESHING == 1
	if (_operationMode == OperationMode::OPERATION_MODE_NORMAL) {
		_mesh->startSync();
	}
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
	if (_tickCount % (60 * 1000 / TICK_INTERVAL_MS) == 0) {
		printLoadStats();
	}

	if (_tickCount % (500 / TICK_INTERVAL_MS) == 0) {
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
	LOGi(FMT_HEADER "running");

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

void Crownstone::handleEvent(event_t& event) {

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
			event.result.dataSize   = sizeof(minFree);
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
			event.result.dataSize   = sizeof(_resetReason);
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
			event.result.dataSize   = sizeof(gpregret);
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
			event.result.dataSize   = sizeof(_ramStats);
			event.result.returnCode = ERR_SUCCESS;
			break;
		}
		default: LOGnone("Event: $typeName(%u)", to_underlying_type(event.type));
	}

	// 	case CS_TYPE::CONFIG_IBEACON_ENABLED: {
	// 		__attribute__((unused)) TYPIFY(CONFIG_IBEACON_ENABLED) enabled =
	// *(TYPIFY(CONFIG_IBEACON_ENABLED)*)event.data;
	// 		// 12-sep-2019 TODO: implement
	// 		LOGw("TODO ibeacon enabled=%i", enabled);
	// 		break;
	// 	}
}

void Crownstone::updateHeapStats() {
	// Don't have to do much, _sbrk() is the best place to keep up the heap end.
	_ramStats.maxHeapEnd   = (uint32_t)getHeapEndMax();
	_ramStats.minFree      = _ramStats.minStackEnd - _ramStats.maxHeapEnd;
	_ramStats.numSbrkFails = getSbrkNumFails();
}

void Crownstone::updateMinStackEnd() {
	void* stackPointer;
	asm("mov %0, sp" : "=r"(stackPointer) : :);
	if ((uint32_t)stackPointer < _ramStats.minStackEnd) {
		_ramStats.minStackEnd = (uint32_t)stackPointer;
	}
}

void Crownstone::printLoadStats() {
	// Log ram usage.
	uint8_t* heapPointer = (uint8_t*)malloc(1);
	void* stackPointer;
	asm("mov %0, sp" : "=r"(stackPointer) : :);
	LOGd("Memory heap=%p, stack=%p", heapPointer, (uint8_t*)stackPointer);
	free(heapPointer);

	LOGi("heapEnd=0x%X maxHeapEnd=0x%X minStackEnd=0x%X minFree=%u sbrkFails=%u",
		 (uint32_t)getHeapEnd(),
		 _ramStats.maxHeapEnd,
		 _ramStats.minStackEnd,
		 _ramStats.minFree,
		 _ramStats.numSbrkFails);

	// Log scheduler usage.
	__attribute__((unused)) uint16_t maxUsed     = app_sched_queue_utilization_get();
	__attribute__((unused)) uint16_t currentFree = app_sched_queue_space_get();
	LOGi("Scheduler current free=%u max used=%u", currentFree, maxUsed);
}

/*
 * Handle bootloader information. If a firmware is just activated, IPC RAM for managing error conditions around
 * reboots (esp. with respect to the microapps) will be cleared. If the IPC version is exactly the same between
 * bootloader and firmware the justActivated flag is subsequently cleared.
 *
 * Caution is required if there's an updateError which is not cleared by the bootloader across reboots. This means
 * that RAM data for the microapps will always be cleared. That subsequently means that misbehaving microapps can not
 * be properly detected.
 *
 * To fix this, it should be possible by the user to clear IPC ram.
 */
void handleBootloaderInfo() {
	bluenet_ipc_data_t ipcData;
	uint8_t dataSize;
	LOGi("Get bootloader IPC data info");
	IpcRetCode ipcCode = getRamData(IPC_INDEX_BOOTLOADER_INFO, ipcData.raw, &dataSize, sizeof(ipcData.raw));
	if (ipcCode != IPC_RET_SUCCESS) {
		LOGw("Bootloader IPC data error: ipcCode=%i", ipcCode);
		return;
	}

	if (ipcData.bootloaderData.ipcDataMajor != g_BLUENET_COMPAT_BOOTLOADER_IPC_RAM_MAJOR) {
		LOGi("Different IPC bootloader major: major=%u required=%u",
			 ipcData.bootloaderData.ipcDataMajor,
			 g_BLUENET_COMPAT_BOOTLOADER_IPC_RAM_MAJOR);
		return;
	}

	// We can change this later to a lower minimal minor version.
	if (ipcData.bootloaderData.ipcDataMinor < g_BLUENET_COMPAT_BOOTLOADER_IPC_RAM_MINOR) {
		LOGi("Too old IPC bootloader minor: minor=%u minimum=%u.",
			 ipcData.bootloaderData.ipcDataMinor,
			 g_BLUENET_COMPAT_BOOTLOADER_IPC_RAM_MINOR);
		return;
	}

	if (ipcData.bootloaderData.justActivated || ipcData.bootloaderData.updateError) {
		LOGi("Clear RAM data for microapps.");
		clearRamData(IPC_INDEX_MICROAPP);
	}

	if (ipcData.bootloaderData.justActivated) {
		LOGi("Clear the just activated flag");
		ipcData.bootloaderData.justActivated = 0;
		// Use the raw buffer, so we keep the possible newer data as well (in case of newer minor version).
		setRamData(IPC_INDEX_BOOTLOADER_INFO, ipcData.raw, dataSize);
	}

	LOGi("Bootloader version: %u.%u.%u-RC%u",
		 ipcData.bootloaderData.bootloaderMajor,
		 ipcData.bootloaderData.bootloaderMinor,
		 ipcData.bootloaderData.bootloaderPatch,
		 ipcData.bootloaderData.bootloaderPrerelease);
	LOGd("Bootloader dfuVersion=%u buildType=%u",
		 ipcData.bootloaderData.dfuVersion,
		 ipcData.bootloaderData.bootloaderBuildType);
}

/**********************************************************************************************************************
 * The main function. Note that this is not the first function called! For starters, if there is a bootloader present,
 * the code within the bootloader has been processed before. But also after the bootloader, the code in
 * cs_sysNrf51.c will set event handlers and other stuff (such as coping with product anomalies, PAN), before calling
 * main. If you enable a new peripheral device, make sure you enable the corresponding event handler there as well.
 *********************************************************************************************************************/

int main() {
	// this enabled the hard float, without it, we get a hardfault
	SCB->CPACR |= (3UL << 20) | (3UL << 22);
	__DSB();
	__ISB();

	atexit(on_exit);

#if CS_SERIAL_NRF_LOG_ENABLED > 0
	NRF_LOG_INIT(NULL);
	NRF_LOG_DEFAULT_BACKENDS_INIT();
	NRF_LOG_INFO("Main");
	NRF_LOG_FLUSH();
#endif

	uint32_t errCode;
	boards_config_t board;
	errCode = configure_board(&board);
	APP_ERROR_CHECK(errCode);

	// Init GPIO pins early in the process!

	if (board.flags.usesNfcPins) {
		cs_ret_code_t retCode = enableNfcPinsAsGpio();
		if (retCode != ERR_SUCCESS) {
			// Not much we can do here.
		}
	}

	//	if (IS_CROWNSTONE(board.deviceType)) {
	// Turn dimmer off.
	if (board.pinDimmer != PIN_NONE) {
		nrf_gpio_cfg_output(board.pinDimmer);
		if (board.flags.dimmerInverted) {
			nrf_gpio_pin_set(board.pinDimmer);
		}
		else {
			nrf_gpio_pin_clear(board.pinDimmer);
		}
	}

	if (board.pinEnableDimmer != PIN_NONE) {
		nrf_gpio_cfg_output(board.pinEnableDimmer);
		nrf_gpio_pin_clear(board.pinEnableDimmer);
	}

	// Relay pins.
	if (board.pinRelayOff != PIN_NONE) {
		nrf_gpio_cfg_output(board.pinRelayOff);
		nrf_gpio_pin_clear(board.pinRelayOff);
	}
	if (board.pinRelayOn != PIN_NONE) {
		nrf_gpio_cfg_output(board.pinRelayOn);
		nrf_gpio_pin_clear(board.pinRelayOn);
	}
	//	}

	if (board.flags.enableUart) {
		initUart(board.pinRx, board.pinTx);
		LOG_FLUSH();
	}

	// Start the watchdog early (after uart, so we can see the logs).
	Watchdog::init();
	Watchdog::start();

	printNfcPins();
	LOG_FLUSH();

	handleBootloaderInfo();

	Crownstone crownstone(board);

	overwrite_hardware_version();

	// init drivers, configure(), create services and chars,
	crownstone.init(0);
	LOG_FLUSH();

	// run forever ...
	crownstone.run();

	return 0;
}
