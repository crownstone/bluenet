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

#if __clang__
#define STRINGIFY(str) #str
#else
#define STRINGIFY(str) str
#endif

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
#include <third/protocol/led_config.h>
#endif

/**********************************************************************************************************************
 * Custom includes
 *********************************************************************************************************************/

#include <cfg/cs_Settings.h>

#include <events/cs_EventDispatcher.h>
#include <events/cs_EventTypes.h>

/**********************************************************************************************************************
 * Main functionality
 *********************************************************************************************************************/

using namespace BLEpp;


#if CHAR_MESHING==1

/**********************************************************************************************************************
 * Interlude for meshing. Will need to be integrated with the code!
 *********************************************************************************************************************/

extern "C" {

#if BOARD==PCA10001
/* configure button interrupt for evkits */
static void gpiote_init(void)
{
  NRF_GPIO->PIN_CNF[BUTTON_0] = (GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos)
                | (GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos)
        //         | (BUTTON_PULL << GPIO_PIN_CNF_PULL_Pos)
                | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos)
                | (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos);

  NRF_GPIO->PIN_CNF[BUTTON_1] = (GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos)
                | (GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos)
         //        | (BUTTON_PULL << GPIO_PIN_CNF_PULL_Pos)
                | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos)
                | (GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos);


  /* GPIOTE interrupt handler normally runs in STACK_LOW priority, need to put it
  in APP_LOW in order to use the mesh API */
  NVIC_SetPriority(GPIOTE_IRQn, 3);

  NVIC_EnableIRQ(GPIOTE_IRQn);
  NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_PORT_Msk;
}

RNG rng;

void GPIOTE_IRQHandler(void)
{
#ifdef STOP_ADV
	Nrf51822BluetoothStack &stack = Nrf51822BluetoothStack::getInstance();

	if (stack.isAdvertising()) {
		LOGi("Stop advertising");
		stack.stopAdvertising();
	}
#endif
 	CMesh & mesh = CMesh::getInstance();
	NRF_GPIOTE->EVENTS_PORT = 0;
	for (uint8_t i = 0; i < 2; ++i)
	{
		if (NRF_GPIO->IN & (1 << (BUTTON_0 + i)))
		{
			LOGi("Button %i pressed", i);
			uint32_t value = mesh.receive(i+1);
//			value = 1 - value;
			value = rng.getRandom8();
//			led_config(i + 1, value);
			LOGi("1");
			mesh.send(i + 1, value);
		}
	}
}

#endif

} // extern "C"

#endif // CHAR_MESHING == 1


/**
 * If UART is enabled this will be the message printed out over a serial connection. Connectors are expensive, so UART
 * is not available in the final product.
 */
void Crownstone::welcome() {
	nrf_gpio_cfg_output(PIN_GPIO_LED0);
	nrf_gpio_pin_set(PIN_GPIO_LED0);
	config_uart();
	_log(INFO, "\r\n");
	BLEutil::print_heap("Heap init");
	BLEutil::print_stack("Stack init");
	LOGd("Bootloader starts at 0x00034000.");
        // To have DFU, keep application limited to (BOOTLOADER_START - APPLICATION_START_CODE) / 2
	// For (0x35000 - 0x16000)/2 this is 0xF800, so from 0x16000 to 0x25800
	// Very probably FLASH (32MB) is not a problem though, but RAM will be (16kB)!
	LOGi("Welcome at the nRF51822 code for meshing.");
	LOGi("Compilation date: %s", STRINGIFY(COMPILATION_TIME));
	LOGi("Compilation time: %s", __TIME__);
}

/**
 * The default name. This can later be altered by the user if the corresponding service and characteristic is enabled.
 */
void Crownstone::setName() {
	// assemble default name from BLUETOOTH_NAME and COMPILATION_TIME
	char devicename[32];
	sprintf(devicename, "%s_%s", STRINGIFY(BLUETOOTH_NAME), STRINGIFY(COMPILATION_TIME));
	// check config (storage) if another name was stored
	std::string device_name;
	// use default name in case no stored name is found
	Storage::getString(Settings::getInstance().getConfig().device_name, device_name, std::string(devicename));
	// assign name
	LOGi("Set name to %s", device_name.c_str());
	_stack->updateDeviceName(device_name); // max len = ble_gap_devname_max_len (31)
	_stack->updateAppearance(BLE_APPEARANCE_GENERIC_TAG);
}

/* Sets default parameters of the Bluetooth connection.
 *
 * Data is transmitted with +4 dBm.
 *
 * On transmission of data within a connection
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
	_stack->setTxPowerLevel(+4);
	_stack->setMinConnectionInterval(16);
	_stack->setMaxConnectionInterval(32);
	_stack->setConnectionSupervisionTimeout(400);
	_stack->setSlaveLatency(10);
	_stack->setAdvertisingInterval(80);
	_stack->setAdvertisingTimeoutSeconds(0);
}

/**
 * This must be called after the SoftDevice has started.
 */
void Crownstone::configDrivers() {
	pwm_config_t pwm_config;
	pwm_config.num_channels = 1;
	pwm_config.gpio_pin[0] = PIN_GPIO_SWITCH;
	pwm_config.mode = PWM_MODE_976;

	PWM::getInstance().init(&pwm_config);

#if BOARD==PCA10001 || BOARD==PCA10000
	nrf_gpio_cfg_output(PIN_GPIO_LED_CON);
#endif
}

void Crownstone::createServices() {
	LOGi("Create all services");

#if GENERAL_SERVICE==1
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

}

void Crownstone::setup() {
	welcome();

	MasterBuffer::getInstance().alloc(MASTER_BUFFER_SIZE);

	EventDispatcher::getInstance().addListener(this);

	LOGi("Create Timer");
	Timer::getInstance();

	// set up the bluetooth stack that controls the hardware.
	_stack = &Nrf51822BluetoothStack::getInstance();

	// configure parameters for the Bluetooth stack
	configStack();

	// start up the softdevice early because we need it's functions to configure devices it ultimately controls.
	// in particular we need it to set interrupt priorities.
	_stack->init();

#if IBEACON==1
	// if enabled, create the iBeacon parameter object which will be used
	// to start advertisement as an iBeacon

	// get values from config
	uint16_t major, minor;
	uint8_t rssi;
	ble_uuid128_t uuid;

	ps_configuration_t cfg = Settings::getInstance().getConfig();
	Storage::getUint16(cfg.beacon.major, major, BEACON_MAJOR);
	Storage::getUint16(cfg.beacon.minor, minor, BEACON_MINOR);
	Storage::getArray(cfg.beacon.uuid.uuid128, uuid.uuid128, ((ble_uuid128_t)UUID(BEACON_UUID)).uuid128, 16);
	Storage::getUint8(cfg.beacon.rssi, rssi, BEACON_RSSI);

	// create ibeacon object
	_beacon = new IBeacon(uuid, major, minor, rssi);
#endif

	// set advertising parameters such as the device name and appearance.
	// Note: has to be called after _stack->init or Storage is initialized too early and won't work correctly
	setName();

	_stack->onConnect([&](uint16_t conn_handle) {
		LOGi("onConnect...");
		// todo this signature needs to change
		//NRF51_GPIO_OUTSET = 1 << PIN_LED;
		// first stop, see https://devzone.nordicsemi.com/index.php/about-rssi-of-ble
		// be neater about it... we do not need to stop, only after a disconnect we do...
		sd_ble_gap_rssi_stop(conn_handle);
		sd_ble_gap_rssi_start(conn_handle);

#if BOARD==PCA10001 || BOARD==PCA10000
		nrf_gpio_pin_set(PIN_GPIO_LED_CON);
#endif
	});
	_stack->onDisconnect([&](uint16_t conn_handle) {
		LOGi("onDisconnect...");
		//NRF51_GPIO_OUTCLR = 1 << PIN_LED;

		// of course this is not nice, but dirty! we immediately start advertising automatically after being
		// disconnected. but for now this will be the default behaviour.

#if BOARD==PCA10001 || BOARD==PCA10000
		nrf_gpio_pin_clear(PIN_GPIO_LED_CON);
#endif

		bool wasScanning = _stack->isScanning();
		_stack->stopScanning();

#if IBEACON==1
		_stack->startIBeacon(_beacon);
#else
		_stack->startAdvertising();
#endif

		if (wasScanning)
			_stack->startScanning();

	});

	createServices();

#if BOARD==CROWNSTONE_SENSOR
	_sensors = new Sensors;
#endif

	// configure drivers
	configDrivers();
	BLEutil::print_heap("Heap drivers: ");
	BLEutil::print_stack("Stack drivers: ");

	// begin sending advertising packets over the air.
#if IBEACON==1
	_stack->startIBeacon(_beacon);
#else
	_stack->startAdvertising();
#endif
	BLEutil::print_heap("Heap adv: ");
	BLEutil::print_stack("Stack adv: ");

#if CHAR_MESHING==1
	#if BOARD==PCA10001
        gpiote_init();
    #endif

    #if BOARD == VIRTUALMEMO
        nrf_gpio_range_cfg_output(7,14);
    #endif
#endif

#if CHAR_MESHING==1
 	CMesh & mesh = CMesh::getInstance();
	mesh.init();
#endif

#if (POWER_SERVICE==1) and (DEFAULT_ON==1)
	LOGi("Set power ON by default");
	nrf_delay_ms(1000);
	_powerService->turnOn();
#else
	LOGi("Set power OFF by default");
#endif

}

void Crownstone::run() {

	_stack->startTicking();

#if (BOARD==CROWNSTONE_SENSOR)
		_sensors->startTicking();
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
	LOGi("handleEvent: %d", evt);
	switch(evt) {
#if IBEACON==1
	case CONFIG_IBEACON_MAJOR: {
		_beacon->setMajor(*(uint32_t*)p_data);
		break;
	}
	case CONFIG_IBEACON_MINOR: {
		_beacon->setMinor(*(uint32_t*)p_data);
		break;
	}
	case CONFIG_IBEACON_UUID: {
		_beacon->setUUID(*(ble_uuid128_t*)p_data);
		break;
	}
	case CONFIG_IBEACON_RSSI: {
		_beacon->setRSSI(*(int8_t*)p_data);
		break;
	}
#endif
	}
}

void on_exit(void) {
	LOGi("PROGRAM TERMINATED");
}

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

	// run forever ...
	crownstone.run();

}
