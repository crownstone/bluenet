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

#define MICRO_VIEW 1

// enable IBeacon functionality (advertises Crownstone as iBeacon)
//#define IBEACON

#ifdef IBEACON
	// define the iBeacon advertisement package parameters
	// the proximity UUID
	#define BEACON_UUID   "ed3a6985-8872-4bb7-b784-c59ef3589844"
	// the major number
	#define BEACON_MAJOR  1
    // the minor number
	#define BEACON_MINOR  5
    // the rssi
	#define BEACON_RSSI   0xc7
#endif

#define DEFAULT_ON

#if __clang__
#define STRINGIFY(str) #str
#else
#define STRINGIFY(str) str
#endif

/**********************************************************************************************************************
 * General includes
 *********************************************************************************************************************/

#include "cs_BluetoothLE.h"

#include <common/cs_Config.h>

#if(NORDIC_SDK_VERSION < 5)
#include "ble_stack_handler.h"
#include "ble_nrf6310_pins.h"
#endif
#include "nrf51_bitfields.h"

extern "C" {
#include "ble_advdata_parser.h"
#include "nrf_delay.h"
}

#include "nordic_common.h"
#include "cs_nRF51822.h"

#include <stdbool.h>
#include <stdint.h>
#include <cstring>
#include <cstdio>

#include "common/cs_Boards.h"

#if(BOARD==PCA10001)
	#include "nrf_gpio.h"
#endif

#if MESHING==1
#include <protocol/cs_Mesh.h>
#endif

#include "drivers/cs_RTC.h"
#include "drivers/cs_ADC.h"
#include "drivers/cs_PWM.h"
#include "drivers/cs_LPComp.h"
#include "drivers/cs_Serial.h"
#include "common/cs_Storage.h"
#include "util/cs_Utils.h"

#if (BOARD==CROWNSTONE_SENSOR)
#include "cs_Sensors.h"
#endif

#if INDOOR_SERVICE==1
#include <services/cs_IndoorLocalisationService.h>
#endif
#if GENERAL_SERVICE==1
#include <services/cs_GeneralService.h>
#endif
#if POWER_SERVICE==1
#include <services/cs_PowerService.h>
#endif

#include "common/cs_MasterBuffer.h"

using namespace BLEpp;

/**********************************************************************************************************************
 * Main functionality
 *********************************************************************************************************************/

/**
 * If UART is enabled this will be the message printed out over a serial connection. Connectors are expensive, so UART
 * is not available in the final product.
 */
void welcome() {
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
void setName(Nrf51822BluetoothStack &stack) {
	char devicename[32];
	sprintf(devicename, "%s_%s", STRINGIFY(BLUETOOTH_NAME), STRINGIFY(COMPILATION_TIME));
	LOGi("Set name to %s", STRINGIFY(BLUETOOTH_NAME));
	stack.setDeviceName(std::string(devicename)) // max len = ble_gap_devname_max_len (31)
		.setAppearance(BLE_APPEARANCE_GENERIC_TAG);
}

/* Sets default parameters of the Bluetooth connection.
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
void configure(Nrf51822BluetoothStack &stack) {
	stack.setTxPowerLevel(-4)
		.setMinConnectionInterval(16)
		.setMaxConnectionInterval(32)
		.setConnectionSupervisionTimeout(400)
		.setSlaveLatency(10)
		.setAdvertisingInterval(100)
		.setAdvertisingTimeoutSeconds(0);
}

/**
 * This must be called after the SoftDevice has started.
 */
void config_drivers() {
	pwm_config_t pwm_config = PWM_DEFAULT_CONFIG;
	pwm_config.num_channels = 1;
	pwm_config.gpio_pin[0] = PIN_GPIO_SWITCH;
	pwm_config.mode = PWM_MODE_976;

	PWM::getInstance().init(&pwm_config);

#if(BOARD==PCA10001)
	nrf_gpio_cfg_output(PIN_GPIO_LED_CON);
#endif
}

#if MESHING==1

/**********************************************************************************************************************
 * Interlude for meshing. Will need to be integrated with the code!
 *********************************************************************************************************************/

extern "C" {

#ifdef BOARD_PCA10001             
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
			value = 1 - value;
			led_config(i + 1, value);
			mesh.send(i + 1, value);
		} 
	} 
}

#endif // nRF6310

} // extern "C"

#endif // MESHING == 1

/**********************************************************************************************************************
 * The main function. Note that this is not the first function called! For starters, if there is a bootloader present,
 * the code within the bootloader has been processed before. But also after the bootloader, the code in 
 * nRF51822.c will set event handlers and other stuff (such as coping with product anomalies, PAN), before calling 
 * main. If you enable a new peripheral device, make sure you enable the corresponding event handler there as well.
 *********************************************************************************************************************/

int main() {
	welcome();

	MasterBuffer::getInstance().alloc(500);

	// set up the bluetooth stack that controls the hardware.
	Nrf51822BluetoothStack &stack = Nrf51822BluetoothStack::getInstance();

	// set advertising parameters such as the device name and appearance.  
	setName(stack);

	// configure parameters for the Bluetooth stack
	configure(stack);

#ifdef IBEACON
	// if enabled, create the iBeacon parameter object which will be used
	// to start advertisment as an iBeacon
	IBeacon beacon(UUID(BEACON_UUID), BEACON_MAJOR, BEACON_MINOR, BEACON_RSSI);
#endif

	// start up the softdevice early because we need it's functions to configure devices it ultimately controls.
	// in particular we need it to set interrupt priorities.
	stack.init();

	stack.onConnect([&](uint16_t conn_handle) {
			LOGi("onConnect...");
			// todo this signature needs to change
			//NRF51_GPIO_OUTSET = 1 << PIN_LED;
			// first stop, see https://devzone.nordicsemi.com/index.php/about-rssi-of-ble
			// be neater about it... we do not need to stop, only after a disconnect we do...
			sd_ble_gap_rssi_stop(conn_handle);
			sd_ble_gap_rssi_start(conn_handle);

#if(BOARD==PCA10001)
			nrf_gpio_pin_set(PIN_GPIO_LED_CON);
#endif
		})
		.onDisconnect([&](uint16_t conn_handle) {
			LOGi("onDisconnect...");
			//NRF51_GPIO_OUTCLR = 1 << PIN_LED;

			// of course this is not nice, but dirty! we immediately start advertising automatically after being
			// disconnected. but for now this will be the default behaviour.

#if(BOARD==PCA10001)
			nrf_gpio_pin_clear(PIN_GPIO_LED_CON);
#endif

			bool wasScanning = stack.isScanning();
			stack.stopScanning();

#ifdef IBEACON
			stack.startIBeacon(beacon);
#else
			stack.startAdvertising();
#endif

			if (wasScanning)
				stack.startScanning();

		});

	LOGi("Start RTC");
	RealTimeClock::getInstance().init();
	RealTimeClock::getInstance().start();

	LOGi("Create all services");

#if INDOOR_SERVICE==1
	// now, build up the services and characteristics.
	IndoorLocalizationService& localizationService = IndoorLocalizationService::createService(stack);
#endif

#if GENERAL_SERVICE==1
	// general services, such as internal temperature, setting names, etc.
	GeneralService& generalService = GeneralService::createService(stack);
#endif

#if POWER_SERVICE==1
	PowerService &powerService = PowerService::createService(stack);
#endif

#if (BOARD==CROWNSTONE_SENSOR)
	Sensors sensors;
#endif

	// configure drivers
	config_drivers();
	BLEutil::print_heap("Heap drivers: ");
	BLEutil::print_stack("Stack drivers: ");

	// begin sending advertising packets over the air.
#ifdef IBEACON
	stack.startIBeacon(beacon);
#else
	stack.startAdvertising();
#endif
	BLEutil::print_heap("Heap adv: ");
	BLEutil::print_stack("Stack adv: ");

#if MESHING==1
    #ifdef BOARD_PCA10001
        gpiote_init();
    #endif

    #if BOARD == VIRTUALMEMO
        nrf_gpio_range_cfg_output(7,14);
    #endif
#endif

#if MESHING==1
 	CMesh & mesh = CMesh::getInstance();
	mesh.init();
#endif
#if POWER_SERVICE==1
#ifdef DEFAULT_ON
	LOGi("Set power ON by default");
	nrf_delay_ms(1000);
	powerService.turnOn();
#else
	LOGi("Set power OFF by default");
#endif
#endif
	LOGi("Running while ticking..");
	//static long int dbg_counter = 0;
	while(1) {
		//LOGd("Tick %li", ++dbg_counter); // we really have to monitor the frequency of our ticks
		
		// The stack tick will halt everything until there is a connection made...
		stack.tick();
#if GENERAL_SERVICE==1
		generalService.tick();
#endif
#if POWER_SERVICE==1
		powerService.tick();
#endif
#if INDOOR_SERVICE==1
		localizationService.tick();
#endif

#if (BOARD==CROWNSTONE_SENSOR)
		sensors.tick();
#endif

//		app_sched_execute();

		nrf_delay_ms(50);
	}
}
