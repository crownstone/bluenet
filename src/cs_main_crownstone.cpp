/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 14 Aug., 2014
 * License: LGPLv3+, Apache, or MIT, your choice
 */

/**********************************************************************************************************************
 * The Crownstone is a high-voltage (domestic) switch. It can be used to combine:
 *   - indoor localization
 *   - building automation
 * It is therefore one of the first real internet-of-things devices entering the market.
 * Read more on: https://dobots.nl/products/crownstone/
 *
 * A large part of the software is open-source. You can find it at: https://github.com/dobots/bluenet. The README
 * file at that repository will get you started.
 *
 * Almost all configuration options should be set in CMakeBuild.config.
 *********************************************************************************************************************/

// temporary defines
#define MESHING_PARALLEL 1

//#define MICRO_VIEW 1
//#define CHANGE_NAME_ON_RESET
//#define CHANGE_MINOR_ON_RESET

/**********************************************************************************************************************
 * General includes
 *********************************************************************************************************************/

#include <cs_Crownstone.h>

#include "cfg/cs_Boards.h"
#include "drivers/cs_RTC.h"
#include "drivers/cs_PWM.h"
#include "util/cs_Utils.h"
#include "drivers/cs_Timer.h"
#include "structs/buffer/cs_MasterBuffer.h"

#if CHAR_MESHING==1
#include <protocol/cs_Mesh.h>
#include <drivers/cs_RNG.h>
#endif

/**********************************************************************************************************************
 * Custom includes
 *********************************************************************************************************************/

#include <cfg/cs_Settings.h>

#include <events/cs_EventDispatcher.h>
#include <events/cs_EventTypes.h>

#include <ble/cs_DoBotsManufac.h>

/**********************************************************************************************************************
 * Main functionality
 *********************************************************************************************************************/

using namespace BLEpp;


/**
 * If UART is enabled this will be the message printed out over a serial connection. Connectors are expensive, so UART
 * is not available in the final product.
 */
void Crownstone::welcome() {
	config_uart();
	_log(INFO, "\r\n");
	BLEutil::print_heap("Heap init");
	BLEutil::print_stack("Stack init");
	// To have DFU, keep application limited to (BOOTLOADER_REGION_START - APPLICATION_START_CODE - DFU_APP_DATA_RESERVED)
	// For (0x38000 - 0x1C000 - 0x400) this is 0x1BC00 (113664 bytes)
	LOGi("Welcome at the nRF51822 code for meshing.");
	LOGi("Compilation date: %s", STRINGIFY(COMPILATION_TIME));
	LOGi("Compilation time: %s", __TIME__);
}

/**
 * The default name. This can later be altered by the user if the corresponding service and characteristic is enabled.
 */
void Crownstone::setName() {
#ifdef CHANGE_NAME_ON_RESET
	uint16_t minor;
	ps_configuration_t cfg = Settings::getInstance().getConfig();
	Storage::getUint16(cfg.beacon.minor, minor, BEACON_MINOR);
	char devicename[32];
	sprintf(devicename, "%s_%d", STRINGIFY(BLUETOOTH_NAME), STRINGIFY(minor));
	std::string device_name = std::string(devicename);
	// End test for wouter
#else

	// assemble default name from BLUETOOTH_NAME and COMPILATION_TIME
	char devicename[32];
	sprintf(devicename, "%s_%s", STRINGIFY(BLUETOOTH_NAME), STRINGIFY(COMPILATION_TIME));
	// check config (storage) if another name was stored
	std::string device_name;
	// use default name in case no stored name is found
	Storage::getString(Settings::getInstance().getConfig().device_name, device_name, std::string(devicename));
#endif
	// assign name
	LOGi("Set name to %s", device_name.c_str());
	_stack->updateDeviceName(device_name); // max len = ble_gap_devname_max_len (31)
	_stack->updateAppearance(BLE_APPEARANCE_GENERIC_TAG);
}

/* Sets default parameters of the Bluetooth connection.
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
 */
void Crownstone::configStack() {
#if LOW_POWER_MODE==0
	_stack->setClockSource(NRF_CLOCK_LFCLKSRC_SYNTH_250_PPM);
#else
	// TODO: depends on board!
//	_stack->setClockSource(NRF_CLOCK_LFCLKSRC_XTAL_50_PPM);
	_stack->setClockSource(CLOCK_SOURCE);
#endif

	_stack->setTxPowerLevel(TX_POWER);
	_stack->setMinConnectionInterval(16);
	_stack->setMaxConnectionInterval(32);
	_stack->setConnectionSupervisionTimeout(400);
	_stack->setSlaveLatency(10);
	_stack->setAdvertisingInterval(ADVERTISEMENT_INTERVAL);
	_stack->setAdvertisingTimeoutSeconds(0);
}

/**
 * This must be called after the SoftDevice has started.
 */
void Crownstone::configDrivers() {
#if PWM_ENABLE==1
	pwm_config_t pwm_config;
	pwm_config.num_channels = 1;
	pwm_config.gpio_pin[0] = PIN_GPIO_SWITCH;
	pwm_config.mode = PWM_MODE_976;
//	pwm_config.mode = PWM_MODE_122;

	PWM::getInstance().init(&pwm_config);

	_temperatureGuard = new TemperatureGuard();
	_temperatureGuard->startTicking();
#endif

#if HARDWARE_BOARD==PCA10001
	nrf_gpio_cfg_output(PIN_GPIO_LED_CON);
#endif

#if HAS_LEDS==1
	// Note: DO NOT USE THEM WHILE SCANNING OR MESHING ...
	nrf_gpio_cfg_output(PIN_GPIO_LED_1);
	nrf_gpio_cfg_output(PIN_GPIO_LED_2);
	// setting the pin makes them turn off ....
	nrf_gpio_pin_set(PIN_GPIO_LED_1);
	nrf_gpio_pin_set(PIN_GPIO_LED_2);
#endif
}

void Crownstone::createServices() {
	LOGi("Create all services");

// should be available always, but for now, only enable if required
#if DEVICE_INFO_SERVICE==1
	_deviceInformationService = new DeviceInformationService();
	_stack->addService(_deviceInformationService);
#endif

#if GENERAL_SERVICE==1 || DEVICE_TYPE==DEVICE_FRIDGE
	// general services, such as internal temperature, setting names, etc.
	_generalService = new GeneralService;
	_stack->addService(_generalService);
#endif

#if INDOOR_SERVICE==1
	// now, build up the services and characteristics.
	_localizationService = new IndoorLocalizationService;
	_stack->addService(_localizationService);
#endif

#if POWER_SERVICE==1
	_powerService = new PowerService;
	_stack->addService(_powerService);
#endif

#if ALERT_SERVICE==1 || DEVICE_TYPE==DEVICE_FRIDGE
	_alertService = new AlertService;
	_stack->addService(_alertService);
#endif

#if SCHEDULE_SERVICE==1
	_scheduleService = new ScheduleService;
	_stack->addService(_scheduleService);
#endif
}

void Crownstone::configure() {

	LOGi("Configure stack ...");

	// configure parameters for the Bluetooth stack
	configStack();

	// start up the softdevice early because we need it's functions to configure devices it ultimately controls.
	// in particular we need it to set interrupt priorities.
	_stack->init();

	LOGi("Loading configuration");

	ps_configuration_t cfg = Settings::getInstance().getConfig();

#if IBEACON==1 || DEVICE_TYPE==DEVICE_DOBEACON
	// if enabled, create the iBeacon parameter object which will be used
	// to start advertisement as an iBeacon

	// get values from config
	uint16_t major, minor;
	uint8_t rssi;
	ble_uuid128_t uuid;

	Storage::getUint16(cfg.beacon.major, major, BEACON_MAJOR);
	Storage::getUint16(cfg.beacon.minor, minor, BEACON_MINOR);
	Storage::getArray(cfg.beacon.uuid.uuid128, uuid.uuid128, ((ble_uuid128_t)UUID(BEACON_UUID)).uuid128, 16);
	Storage::getUint8(cfg.beacon.rssi, rssi, BEACON_RSSI);

#ifdef CHANGE_MINOR_ON_RESET
	minor++;
	LOGi("Increase minor to %d", minor);
	Settings::getInstance().writeToStorage(CONFIG_IBEACON_MINOR, (uint8_t*)&minor, 2);
#endif

	// create ibeacon object
	_beacon = new IBeacon(uuid, major, minor, rssi);
#endif

	// set advertising parameters such as the device name and appearance.
	// Note: has to be called after _stack->init or Storage is initialized too early and won't work correctly
	setName();

	// Set the stored tx power
	int8_t txPower;
	Storage::getInt8(cfg.txPower, txPower, TX_POWER);
	_stack->setTxPowerLevel(txPower);

	// Set the stored advertisement interval
	uint16_t advInterval;
	Storage::getUint16(cfg.advInterval, advInterval, ADVERTISEMENT_INTERVAL);
	_stack->setAdvertisingInterval(advInterval);

#if ENCRYPTION==1
	uint8_t passkey[BLE_GAP_PASSKEY_LEN];
	Storage::getArray(cfg.passkey, passkey, (uint8_t*)STATIC_PASSKEY, BLE_GAP_PASSKEY_LEN);
	_stack->setPasskey(passkey);
#endif

	LOGi("... done");
}

void Crownstone::setup() {
	welcome();

	MasterBuffer::getInstance().alloc(MASTER_BUFFER_SIZE);

	EventDispatcher::getInstance().addListener(this);

	LOGi("Create Timer");
	Timer::getInstance();

	// set up the bluetooth stack that controls the hardware.
	_stack = &Nrf51822BluetoothStack::getInstance();

	// configuration has to be done after the stack was created!
	configure();

	uint16_t bootDelay;
	Storage::getUint16(Settings::getInstance().getConfig().bootDelay, bootDelay, BOOT_DELAY);
	nrf_delay_ms(bootDelay);

	_stack->onConnect([&](uint16_t conn_handle) {
		LOGi("onConnect...");
		// todo this signature needs to change
		//NRF51_GPIO_OUTSET = 1 << PIN_LED;
		// first stop, see https://devzone.nordicsemi.com/index.php/about-rssi-of-ble
		// be neater about it... we do not need to stop, only after a disconnect we do...
#if RSSI_ENABLE==1

		sd_ble_gap_rssi_stop(conn_handle);

#if (SOFTDEVICE_SERIES == 130 && SOFTDEVICE_MAJOR == 1 && SOFTDEVICE_MINOR == 0) || \
	(SOFTDEVICE_SERIES == 110 && SOFTDEVICE_MAJOR == 8)
		sd_ble_gap_rssi_start(conn_handle, 0, 0);
#else
		sd_ble_gap_rssi_start(conn_handle);
#endif
#endif

#if HARDWARE_BOARD==PCA10001
		nrf_gpio_pin_set(PIN_GPIO_LED_CON);
#endif

	});
	_stack->onDisconnect([&](uint16_t conn_handle) {
		LOGi("onDisconnect...");
		//NRF51_GPIO_OUTCLR = 1 << PIN_LED;

		// of course this is not nice, but dirty! we immediately start advertising automatically after being
		// disconnected. but for now this will be the default behaviour.

#if HARDWARE_BOARD==PCA10001
		nrf_gpio_pin_clear(PIN_GPIO_LED_CON);
#endif

		bool wasScanning = _stack->isScanning();
		_stack->stopScanning();

		startAdvertising();

		if (wasScanning)
			_stack->startScanning();

	});

	createServices();

	_stack->device_manager_init();

#if (HARDWARE_BOARD==CROWNSTONE_SENSOR || HARDWARE_BOARD==NORDIC_BEACON)
	_sensors = new Sensors;
#endif

#if DEVICE_TYPE==DEVICE_FRIDGE
	_fridge = new Fridge;
	_fridge->startTicking();
#endif

	// configure drivers
	configDrivers();
	BLEutil::print_heap("Heap drivers: ");
	BLEutil::print_stack("Stack drivers: ");

#if CHAR_MESHING==1
//	#if HARDWARE_BOARD==PCA10001
//        gpiote_init();
//    #endif

    #if HARDWARE_BOARD == VIRTUALMEMO
        nrf_gpio_range_cfg_output(7,14);
    #endif

 	CMesh & mesh = CMesh::getInstance();
	mesh.init();
	BLEutil::print_heap("Heap mesh: ");
	BLEutil::print_stack("Stack mesh: ");
#endif

	// Begin sending advertising packets over the air.
	// This should be done after initialization of the mesh
	startAdvertising();
	BLEutil::print_heap("Heap adv: ");
	BLEutil::print_stack("Stack adv: ");

#if (POWER_SERVICE==1)
#if (DEFAULT_ON==1)
	LOGi("Set power ON by default");
	nrf_delay_ms(1000);
	_powerService->turnOn();
#elif (DEFAULT_ON==0)
	LOGi("Set power OFF by default");
	nrf_delay_ms(1000);
	_powerService->turnOff();
#endif
#endif

}

// start advertising. the advertisment package depends on the device type,
// and if IBEACON is enabled
void Crownstone::startAdvertising() {
#if IBEACON==1 || DEVICE_TYPE==DEVICE_DOBEACON
	_stack->startIBeacon(_beacon, DEVICE_TYPE);
#else
	_stack->startAdvertising(DEVICE_TYPE);
#endif
}

void Crownstone::run() {

	_stack->startTicking();

#if CHAR_MESHING==1
	CMesh::getInstance().startTicking();
#endif

#if (HARDWARE_BOARD==CROWNSTONE_SENSOR || HARDWARE_BOARD==NORDIC_BEACON)
		_sensors->startTicking();
#endif

#if POWER_SERVICE==1
//	_powerService->startStaticSampling();
#endif

	while(1) {

		app_sched_execute();

#if(NORDIC_SDK_VERSION > 5)
		BLE_CALL(sd_app_evt_wait, ());
#else
		BLE_CALL(sd_app_event_wait, () );
#endif

	}
}

void Crownstone::handleEvent(uint16_t evt, void* p_data, uint16_t length) {

//	LOGi("handleEvent: %d", evt);

	bool restartAdvertising = false;
	switch(evt) {

#if IBEACON==1 || DEVICE_TYPE==DEVICE_DOBEACON
	case CONFIG_IBEACON_MAJOR: {
		_beacon->setMajor(*(uint32_t*)p_data);
		restartAdvertising = true;
		break;
	}
	case CONFIG_IBEACON_MINOR: {
		_beacon->setMinor(*(uint32_t*)p_data);
		restartAdvertising = true;
		break;
	}
	case CONFIG_IBEACON_UUID: {
		_beacon->setUUID(*(ble_uuid128_t*)p_data);
		restartAdvertising = true;
		break;
	}
	case CONFIG_IBEACON_RSSI: {
		_beacon->setRSSI(*(int8_t*)p_data);
		restartAdvertising = true;
		break;
	}
#endif
	case CONFIG_TX_POWER: {
//		LOGd("setTxPowerLevel %d", *(int8_t*)p_data);
		_stack->setTxPowerLevel(*(int8_t*)p_data);

		break;
	}
	case CONFIG_ADV_INTERVAL: {
		_stack->setAdvertisingInterval(*(uint32_t*)p_data);
		restartAdvertising = true;
		break;
	}
	case CONFIG_PASSKEY: {
		_stack->setPasskey((uint8_t*)p_data);
		break;
	}

#if ALERT_SERVICE==1
	case EVT_ENV_TEMP_LOW: {
		if (_alertService != NULL) {
			_alertService->addAlert(ALERT_TEMP_LOW_POS);
//			_alertService->alert(ALERT_TEMP_LOW);
		}
		break;
	}
	case EVT_ENV_TEMP_HIGH: {
		if (_alertService != NULL) {
			_alertService->addAlert(ALERT_TEMP_HIGH_POS);
//			_alertService->alert(ALERT_TEMP_HIGH);
		}
		break;
	}
#endif

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
			startAdvertising();
		}
		break;
	}

	}

	if (restartAdvertising && _stack->isAdvertising()) {
		_stack->stopAdvertising();
		startAdvertising();
	}
}

void on_exit(void) {
	LOGf("PROGRAM TERMINATED");
}


/**********************************************************************************************************************/
#if CHAR_MESHING==1
uint32_t appTimerId = -1;

uint32_t count[2] = {};
//uint8_t idx = -1;
uint8_t idx = 0;

void sendMesh(void* ptr) {
//	idx = (idx + 1) % 2;

	LOGi("<< ch: %d", idx * 2 + 1);
	LOGi("<< count: %d", count[idx]);
	hub_mesh_message_t message;
	memset(&message, 0, sizeof(message));
	message.testMsg.counter = count[idx]++;
	message.header.messageType = 102;

	LOGi("message data:");
	BLEutil::printArray(&message, sizeof(message));
	CMesh::getInstance().send(idx * 2 + 1, &message, sizeof(message));

	Timer::getInstance().start(appTimerId, HZ_TO_TICKS(1), NULL);
}
#endif

/**********************************************************************************************************************
 * The main function. Note that this is not the first function called! For starters, if there is a bootloader present,
 * the code within the bootloader has been processed before. But also after the bootloader, the code in
 * cs_sysNrf51.c will set event handlers and other stuff (such as coping with product anomalies, PAN), before calling
 * main. If you enable a new peripheral device, make sure you enable the corresponding event handler there as well.
 *********************************************************************************************************************/

int main() {
	atexit(on_exit);

	Crownstone crownstone;

	// setup crownstone ...
	crownstone.setup();

#if CHAR_MESHING==1
//	Timer::getInstance().createSingleShot(appTimerId, (app_timer_timeout_handler_t)sendMesh);
//	Timer::getInstance().start(appTimerId, APP_TIMER_TICKS(1, APP_TIMER_PRESCALER), NULL);
#endif

	// run forever ...
	crownstone.run();

}
