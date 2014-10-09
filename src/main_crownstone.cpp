/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 14 Aug., 2014
 * License: LGPLv3+
 */

#include "Pool.h"
#include "BluetoothLE.h"
#include "ble_error.h"

#if(NORDIC_SDK_VERSION < 5)
	#include "ble_stack_handler.h"
	#include "ble_nrf6310_pins.h"
#endif
#include "nrf51_bitfields.h"

#include "nordic_common.h"
#include "nRF51822.h"

#include <stdbool.h>
#include <stdint.h>
#include <cstring>

using namespace BLEpp;

// on the RFduino
#define PIN_RED              2                   // this is GPIO 2 (bottom pin)
#define PIN_GREEN            3                   // this is GPIO 3 (second pin)
#define PIN_BLUE             4                   // this is GPIO 4 (third pin)

#define PIN_LED              0                   // this is GPIO 0

#define BINARY_LED

// An RGB led as with the rfduino requires a sine wave, and thus a PWM signal
//#define RGB_LED

//#define MOTOR_CONTROL


//
#define INDOOR_SERVICE

// the characteristics that need to be included
#define NUMBER_CHARAC
#define CONTROL_CHARAC

#define PIN_MOTOR            6                   // this is GPIO 6 (fifth pin)

/** Example that sets up the Bluetooth stack with two characteristics:
 *   one textual characteristic available for write and one integer characteristic for read.
 * See documentation for a detailed discussion of what's going on here.
 **/
int main() {

	NRF51_GPIO_DIRSET = 3<<8;
	NRF51_UART_ENABLE = 0b100;

	// Configure UART pins:    GPIO   UART
	NRF51_UART_PSELRXD = 16;  	// P0.11  RXD
	NRF51_UART_PSELTXD = 17;   	// P0.09  TXD
	NRF51_UART_PSELRTS = 18;   	// P0.08  RTS
	NRF51_UART_PSELCTS = 19;  	// P0.10  CTS

	NRF51_UART_CONFIG = NRF51_UART_CONFIG_HWFC_ENABLED; // enable hardware flow control.
	NRF51_UART_BAUDRATE = 38400;
	NRF51_UART_STARTTX = 1;

	const char* hello = "Hello, world.\n";	
	uint8_t len = strlen(hello);

	for(int i = 0; i < len; ++i) {
		NRF51_UART_TXD = (uint8_t)hello[i];
		while(!NRF51_UART_TXDRDY) /* wait */;
		NRF51_UART_TXDRDY = 0;
	}

#ifdef BINARY_LED
	uint32_t bin_counter = 0;
	NRF51_GPIO_DIRSET = 1 << PIN_LED; // set pins to output
	NRF51_GPIO_OUTCLR = 1 << PIN_LED; // pin low, led goes off
	NRF51_GPIO_OUTSET = 1 << PIN_LED; // pin low, led goes on
#endif
	//	tick_init();
	//	pwm_init(PIN_GREEN, 1);
	//analogWrite(PIN_GREEN, 50);
	//	volatile int i = 0;
	//	while(1) {
	//		i++;	
	//	};
#ifdef RGB_LED
	uint32_t rgb_counter = 0;
	nrf_pwm_config_t pwm_config = PWM_DEFAULT_CONFIG;

	pwm_config.mode             = PWM_MODE_LED_100;
	pwm_config.num_channels     = 3;
	pwm_config.gpio_num[0]      = PIN_RED;
	pwm_config.gpio_num[1]      = PIN_GREEN;
	pwm_config.gpio_num[2]      = PIN_BLUE;    

	// Initialize the PWM library
	nrf_pwm_init(&pwm_config);
#endif

#ifdef MOTOR_CONTROL
	uint32_t mtr_counter = 50;
//	uint32_t direction = 1;
	nrf_pwm_config_t pwm_config = PWM_DEFAULT_CONFIG;

	// Set PWM to motor controller with 0-100 resolution, 20kHz PWM frequency, 2MHz timer frequency
	pwm_config.mode             = PWM_MODE_MTR_100;
	pwm_config.num_channels     = 1;
	pwm_config.gpio_num[0]      = PIN_MOTOR;

	// Initialize the PWM library
	nrf_pwm_init(&pwm_config);
#endif
	//NRF51_GPIO_OUTSET = 1 << PIN_GREEN; // set pins high -- LED off
	//NRF51_GPIO_OUTCLR = 1 << PIN_GREEN; // set red led on.

	// Memory pool of 30 blocks of 30 bytes each, this already crashes the thing...
	PoolImpl<30> pool;

	// Set up the bluetooth stack that controls the hardware.
	Nrf51822BluetoothStack stack(pool);

	// Set advertising parameters such as the device name and appearance.  These values will
	stack.setDeviceName(std::string("Uart"))
		// controls how device appears in GUI.
		.setAppearance(BLE_APPEARANCE_GENERIC_TAG);
	//	 .setUUID(UUID("00002220-0000-1000-8000-00805f9b34fb"));

	// .setUUID(UUID("20CC0123-B57E-4365-9F35-31C9227D4C4B"));

	// Set connection parameters.  These trade off responsiveness and range for battery life.  See Apple Bluetooth Accessory Guidelines for details.
	// You could omit these; there are reasonable defaults that support medium throughput and long battery life.
	//   interval is set from 20 ms to 40 ms
	//   slave latency is 10
	//   supervision timeout multiplier is 400
	stack.setTxPowerLevel(-4)
		.setMinConnectionInterval(16)
		.setMaxConnectionInterval(32)
		.setConnectionSupervisionTimeout(400)
		.setSlaveLatency(10)
		.setAdvertisingInterval(1600)
		// Advertise forever.  You may instead want to have a button press or something begin
		// advertising for a set period of time, in order to save battery.
		.setAdvertisingTimeoutSeconds(0);

	// start up the softdevice early because we need it's functions to configure devices it ultimately controls.
	// In particular we need it to set interrupt priorities.
	stack.init();


	// Optional notification when radio is active.  Could be used to flash LED.
	stack.onRadioNotificationInterrupt(800, [&](bool radio_active){
			// NB: this executes in interrupt context, so make any global variables volatile.
			if (radio_active) {
			// radio is about to turn on.  Turn on LED.
			// NRF51_GPIO_OUTSET = 1 << Pin_LED;
			} else {
			// radio has turned off.  Turn off LED.
			// NRF51_GPIO_OUTCLR = 1 << Pin_LED;
			}
			});

	//uint32_t err_code = NRF_SUCCESS;

	stack.onConnect([&](uint16_t conn_handle) {
			// A remote central has connected to us.  do something.
			// TODO this signature needs to change
			//NRF51_GPIO_OUTSET = 1 << PIN_LED;
			uint32_t err_code __attribute__((unused)) = NRF_SUCCESS;
			// first stop, see https://devzone.nordicsemi.com/index.php/about-rssi-of-ble
			// be neater about it... we do not need to stop, only after a disconnect we do...
			err_code = sd_ble_gap_rssi_stop(conn_handle);
			err_code = sd_ble_gap_rssi_start(conn_handle);
			})
	.onDisconnect([&](uint16_t conn_handle) {
			// A remote central has disconnected from us.  do something.
			//NRF51_GPIO_OUTCLR = 1 << PIN_LED;
			});

	//Service& generalService = stack.createService();
	//Service& batteryService = stack.createBatteryService();


#ifdef INDOOR_SERVICE
	// Now, build up the services and characteristics.
	Service& localizationService = stack.createIndoorLocalizationService();


#ifdef NUMBER_CHARAC
	// Create a characteristic of type uint8_t (unsigned one byte integer).
	// This characteristic is by default read-only and
	Characteristic<uint8_t>& intChar = localizationService.createCharacteristic<uint8_t>()
		.setUUID(UUID(localizationService.getUUID(), 0x125))  // based off the UUID of the service.
		.setName("number");
#endif // _NUMBER_CHARAC

#ifdef CONTROL_CHARAC
	localizationService.createCharacteristic<std::string>()
		.setUUID(UUID(localizationService.getUUID(), 0x124))
		//		.setName("number input")
		.setName("text")
		.setDefaultValue("")
		.setWritable(true)
		.onWrite([&](const std::string& value) -> void {
				//.onWrite([&](const uint8_t& value) -> void {
				// set the value of the "number" characteristic to the value written to the text characteristic.
				//int nr = value;
#ifdef BINARY_LED
			bin_counter++;
			if (bin_counter % 2) {
				NRF51_GPIO_OUTSET = 1 << PIN_LED; // pin high, led goes on
			} else {
				NRF51_GPIO_OUTCLR = 1 << PIN_LED; // pin low, led goes off
			}
#endif
			for(int i = 0; i < len; ++i) {
				NRF51_UART_TXD = (uint8_t)hello[i];
				while(!NRF51_UART_TXDRDY) /* wait */;
				NRF51_UART_TXDRDY = 0;
			}

#ifdef RGB_LED
			int nr = atoi(value.c_str());
#ifdef NUMBER_CHARAC
			intChar = nr;
#endif
			//	__asm("BKPT");

			//				analogWrite(PIN_RED, nr);
			//
			// Update the 3 outputs with out of phase sine waves
			rgb_counter = nr;
			if (rgb_counter > 100) rgb_counter = 100;
			nrf_pwm_set_value(0, sin_table[rgb_counter]);
			nrf_pwm_set_value(1, sin_table[(rgb_counter + 33) % 100]);
			nrf_pwm_set_value(2, sin_table[(rgb_counter + 66) % 100]);

			// Add a delay to control the speed of the sine wave
			nrf_delay_us(8000);
#endif

#ifdef MOTOR_CONTROL
			int nr = atoi(value.c_str());
#ifdef NUMBER_CHARAC
			intChar = nr;
#endif
			mtr_counter = (int32_t) nr;	
			// Update the output with out of phase sine waves
			//mtr_counter = nr;
			if (mtr_counter > 100) mtr_counter = 100;
			if (mtr_counter < 50) mtr_counter = 50;
/*
			if (mtr_counter == 100) {
				direction = 1;
			}
			if (mtr_counter == 50) {
				direction = -1;
			}
			mtr_counter += direction;
			*/
			nrf_pwm_set_value(0, mtr_counter);
			// Add a delay to control the speed of the sine wave
			nrf_delay_us(8000);
#endif
		});
#endif // _CONTROL_CHARAC
#endif

		// Begin sending advertising packets over the air.  Again, may want to trigger this from a button press to save power.
		stack.startAdvertising();
		while(1) {
			// Deliver events from the Bluetooth stack to the callbacks defined above.
			//		analogWrite(PIN_LED, 50);
			stack.loop();
		}
}
