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

// #define IBEACON

/**********************************************************************************************************************
 * General includes
 *********************************************************************************************************************/

#include "BluetoothLE.h"

#if(NORDIC_SDK_VERSION < 5)
#include "ble_stack_handler.h"
#include "ble_nrf6310_pins.h"
#endif
#include "nrf51_bitfields.h"

extern "C" {
#include "ble_advdata_parser.h"
#include "nrf_delay.h"
#include "app_scheduler.h"
}

#include "nordic_common.h"
#include "nRF51822.h"

#include <stdbool.h>
#include <stdint.h>
#include <cstring>
#include <cstdio>

#include <common/cs_boards.h>

#if(BOARD==PCA10001)
	#include "nrf_gpio.h"
#endif

#include <drivers/nrf_rtc.h>
#include <drivers/nrf_adc.h>
#include <drivers/nrf_pwm.h>
#include <drivers/LPComp.h>
#include <drivers/serial.h>
#include <common/storage.h>

#if MESHING==1
extern "C" {
#include <protocol/rbc_mesh.h>
#include <protocol/rbc_mesh_common.h>
#include <protocol/led_config.h>
}
#endif

#if INDOOR_SERVICE==1
#include <services/IndoorLocalisationService.h>
#endif
#if GENERAL_SERVICE==1
#include <services/GeneralService.h>
#endif
#if POWER_SERVICE==1
#include <services/PowerService.h>
#endif

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
	uint8_t *p = (uint8_t*)malloc(1);
	LOGd("Start of heap %p", p);
	free(p);
	LOGd("Bootloader starts at 0x00035000.");
        // To have DFU, keep application limited to (BOOTLOADER_START - APPLICATION_START_CODE) / 2
	// For (0x35000 - 0x16000)/2 this is 0xF800, so from 0x16000 to 0x25800 
	// Very probably FLASH (32MB) is not a problem though, but RAM will be (16kB)!
	LOGi("Welcome at the nRF51822 code for meshing.");
	LOGi("Compilation date: %s", COMPILATION_TIME);
	LOGi("Compilation time: %s", __TIME__);
}

/**
 * The default name. This can later be altered by the user if the corresponding service and characteristic is enabled.
 */
void setName(Nrf51822BluetoothStack &stack) {
	char devicename[32];
	sprintf(devicename, "%s_%s", BLUETOOTH_NAME, COMPILATION_TIME);
	stack.setDeviceName(std::string(devicename)) // max len = ble_gap_devname_max_len (31)
		.setAppearance(BLE_APPEARANCE_GENERIC_TAG);
}

void configure(Nrf51822BluetoothStack &stack) {
	// set connection parameters.  these trade off responsiveness and range for battery life. 
	// see apple bluetooth accessory guidelines for details.
	// you could omit these; there are reasonable defaults that support medium throughput and long battery life.
	//   interval is set from 20 ms to 40 ms
	//   slave latency is 10
	//   supervision timeout multiplier is 400
	stack.setTxPowerLevel(-4)
		.setMinConnectionInterval(16)
		.setMaxConnectionInterval(32)
		.setConnectionSupervisionTimeout(400)
		.setSlaveLatency(10)
		.setAdvertisingInterval(1600)
		// advertise forever.  you may instead want to have a button press or something begin
		// advertising for a set period of time, in order to save battery.
		.setAdvertisingTimeoutSeconds(0);

}

/**
 * This must be called after the SoftDevice has started.
 */
void config_drivers() {
	// TODO: Dominik, can we connect the adc init call with the characteristic / services
	//   that actually use it? if there is no service that uses it there is no reason to
	//   allocate space for it
//	ADC::getInstance().init(PIN_AIN_ADC); // Moved to PowerService.cpp

	pwm_config_t pwm_config = PWM_DEFAULT_CONFIG;
	pwm_config.num_channels = 1;
	pwm_config.gpio_pin[0] = PIN_GPIO_SWITCH;
	pwm_config.mode = PWM_MODE_LED_255;

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

/**
 * Function to test mesh functionality. We have to figure out if we have to enable the radio first, and that kind of
 * thing.
 */
void setup_mesh() {
	LOGi("For now we are testing the meshing functionality.");
	nrf_gpio_pin_clear(PIN_GPIO_LED0);

	rbc_mesh_init_params_t init_params;

	init_params.access_addr = 0xA541A68F;
	init_params.adv_int_ms = 100;
	init_params.channel = 38;
	init_params.handle_count = 2;
	init_params.packet_format = RBC_MESH_PACKET_FORMAT_ORIGINAL;
	init_params.radio_mode = RBC_MESH_RADIO_MODE_BLE_1MBIT;

	LOGi("Call rbc_mesh_init");
	uint8_t error_code;
	// checks if softdevice is enabled etc.
	error_code = rbc_mesh_init(init_params);
	APP_ERROR_CHECK(error_code);

	LOGi("Call rbc_mesh_value_enable");
	error_code = rbc_mesh_value_enable(1);
	APP_ERROR_CHECK(error_code);
	error_code = rbc_mesh_value_enable(2);
	APP_ERROR_CHECK(error_code);

#if MESHING_PARALLEL == 0
	sd_nvic_EnableIRQ(SD_EVT_IRQn);	
	LOGi("Start waiting for events.");
	while (true)
	{
		sd_app_evt_wait();
	}
#endif
}

/**
 * @brief Softdevice event handler
 *
 * Currently this interrupt screws up the main program. So apparently we have to set priorities somewhere or to tell
 * the program that it is entering a protected region or something like that.
 */               
void SD_EVT_IRQHandler(void)
{   
	// do not print in IRQ handler, use LEDs if you have to
//	LOGd("Received SoftDevice event, call rbc_mesh_sd_irq_handler.");
	rbc_mesh_sd_irq_handler(); 
} 

//#ifdef BOARD_PCA10001             
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
                                                            
//#endif                             
                              
void GPIOTE_IRQHandler(void)                       
{      
#ifdef STOP_ADV
	Nrf51822BluetoothStack &stack = Nrf51822BluetoothStack::getInstance();
	
	if (stack.isAdvertising()) {
		LOGi("Stop advertising");
		stack.stopAdvertising();
	}
#endif
	NRF_GPIOTE->EVENTS_PORT = 0;                      
	for (uint8_t i = 0; i < 2; ++i)                      
	{
		if (NRF_GPIO->IN & (1 << (BUTTON_0 + i)))                  
		{                            
			LOGi("Button %i pressed", i); 
			uint8_t val[28];                       
			uint16_t len;                        
			APP_ERROR_CHECK(rbc_mesh_value_get(i + 1, val, &len, NULL));            
			LOGi("Current mesh data of %i = %i", i + 1, val[0]);
			val[0] = !val[0];                   
			led_config(i + 1, val[0]);                     
			LOGi("Set mesh data %i to %i", i+1, val[0]);
			APP_ERROR_CHECK(rbc_mesh_value_set(i + 1, &val[0], 1));              
		}                
	} 
}


}

#endif


/**********************************************************************************************************************
 * The main function. Note that this is not the first function called! For starters, if there is a bootloader present,
 * the code within the bootloader has been processed before. But also after the bootloader, the code in 
 * nRF51822.c will set event handlers and other stuff (such as coping with product anomalies, PAN), before calling 
 * main. If you enable a new peripheral device, make sure you enable the corresponding event handler there as well.
 *********************************************************************************************************************/

int main() {
	welcome();

	// set up the bluetooth stack that controls the hardware.
	Nrf51822BluetoothStack &stack = Nrf51822BluetoothStack::getInstance();

	// set advertising parameters such as the device name and appearance.  
	setName(stack);

	// configure parameters for the Bluetooth stack
	configure(stack);

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

			stack.stopScanning();

#ifdef IBEACON
			stack.startIBeacon();
#else
			stack.startAdvertising();
#endif
		});

	// Create ADC object
	// TODO: make service which enables other services and only init ADC when necessary
	//	ADC::getInstance();
	//	LPComp::getInstance();

	// Scheduler must be initialized before persistent memory
	const uint16_t max_size = 32;
	const uint16_t queue_size = 4;
	APP_SCHED_INIT(max_size, queue_size);

	// Create persistent memory object
	// TODO: make service which enables other services and only init persistent memory when necessary
	//	Storage::getInstance().init(32);

	//	RealTimeClock::getInstance();

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

	// configure drivers
	config_drivers();

	// begin sending advertising packets over the air.
#ifdef IBEACON
	stack.startIBeacon();
#else
	stack.startAdvertising();
#endif

#if MESHING==1
	gpiote_init();
#endif

#if MESHING==1
	setup_mesh();
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

		app_sched_execute();

		nrf_delay_ms(50);
	}
}
