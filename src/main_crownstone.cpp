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
#include "serial.h"

#include "nrf_adc.h"

#include <stdbool.h>
#include <stdint.h>
#include <cstring>
#include <cmath>

using namespace BLEpp;

//#define NRF6310_BOARD

#define SHUNT_VALUE 120 // Resistance of the shunt in mili ohm

// on the RFduino
#define PIN_RED              2                   // this is GPIO 2 (bottom pin)
#define PIN_GREEN            3                   // this is GPIO 3 (second pin)
#define PIN_BLUE             4                   // this is GPIO 4 (third pin)

#ifdef NRF6310_BOARD
#define PIN_LED              8                   // this is P1.0
#define PIN_ADC              2                   // AIN2 is P0.1
#else
#define PIN_LED              3                   // this is GPIO 3
#define PIN_ADC2              5                   // AIN5 is pin 4
#define PIN_ADC             6                   // AIN6 is pin 5
#endif

// this is the switch on the 220V plug!
//#define BINARY_LED

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
	int personal_threshold_level;
#ifdef BINARY_LED
	uint32_t bin_counter = 0;
//	NRF51_GPIO_DIRSET = 1 << PIN_LED; // set pins to output
//	NRF51_GPIO_OUTCLR = 1 << PIN_LED; // pin low, led goes off
//	NRF51_GPIO_OUTSET = 1 << PIN_LED; // pin low, led goes on

	// Configure LED-pins as outputs.
	nrf_gpio_cfg_output(PIN_LED);
	nrf_gpio_pin_write(PIN_LED, 1); // led goes on
#endif

	// Configure current sensor pin as input
	// NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_NOPULL
	// NRF_GPIO_PIN_NOSENSE, NRF_GPIO_PIN_SENSE_LOW, NRF_GPIO_PIN_SENSE_HIGH
//	nrf_gpio_cfg_sense_input(PIN_CURRENT_SENS_1, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_HIGH);


	config_uart();
	const char* hello = "Welcome at the nRF51822 code for meshing.\r\n";
	write(hello);

	char text[64];
	sprintf(text, "Compilation time: %s \r\n", COMPILATION_TIME);
	write(text);

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
//	stack.setDeviceName(std::string("Crown_bart")) // Max len = BLE_GAP_DEVNAME_MAX_LEN (31)
	char deviceName[32];
	sprintf(deviceName, "Crow_%s", COMPILATION_TIME);
//	std::string devName = std::string("2014-10-27:14");
//	sprintf(text, "devname len: %i\r\n", devName.length());
//	write(text);
	sprintf(text, "devname len: %i\r\n", strlen(deviceName));
	write(text);
	stack.setDeviceName(std::string(deviceName)) // Max len = BLE_GAP_DEVNAME_MAX_LEN (31)
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
	// This characteristic is by default read-only (for the user)
	// Note that in the next characteristic this variable intChar is set! 
	Characteristic<uint64_t>& intChar = localizationService.createCharacteristic<uint64_t>()
		.setUUID(UUID(localizationService.getUUID(), 0x125))  // based off the UUID of the service.
		.setDefaultValue(66)
		.setName("number");
#endif // _NUMBER_CHARAC


	Characteristic<uint64_t>& intChar2 = localizationService.createCharacteristic<uint64_t>()
		.setUUID(UUID(localizationService.getUUID(), 0x121))  // based off the UUID of the service.
		.setDefaultValue(66)
		.setName("number2");


#ifdef CONTROL_CHARAC
	localizationService.createCharacteristic<std::string>()
		.setUUID(UUID(localizationService.getUUID(), 0x124))
		//		.setName("number input")
		.setName("text")
		.setDefaultValue("")
		.setWritable(true)
		.onWrite([&](const std::string& value) -> void {
			std::string msg = std::string("Received message: ") + value + std::string("\r\n");
			const char* received = msg.c_str();
			write(received);
				//.onWrite([&](const uint8_t& value) -> void {
				// set the value of the "number" characteristic to the value written to the text characteristic.
				//int nr = value;


			uint64_t rms_sum = 0;
			uint32_t voltage_min = 0xFFFFFFFF;
			uint32_t voltage_max = 0;
			// Start reading adc
			uint32_t voltage;
			for (uint32_t i=0; i<100*1000; ++i) {
				nrf_adc_read(PIN_ADC2, &voltage);
				rms_sum += voltage*voltage;
				if (voltage < voltage_min)
					voltage_min = voltage;
				if (voltage > voltage_max)
					voltage_max = voltage;
			}
			uint32_t rms = sqrt(rms_sum/(100*1000));

			// 8A max --> 0.96V max --> voltage value is max 960000, which is smaller than 2^32

			// measured voltage goes from 0-3.6V(due to 1/3 multiplier), measured as 0-255(8bit) or 0-1024(10bit)
//			voltage = voltage*1000*1200*3/255; // nV   8 bit
			voltage     = voltage    *1000*1200*3/1024; // nV   10 bit
			voltage_min = voltage_min*1000*1200*3/1024;
			voltage_max = voltage_max*1000*1200*3/1024;
			rms         = rms        *1000*1200*3/1024;

			uint16_t current = rms / SHUNT_VALUE; // mA

			char text[128];
			sprintf(text, "voltage(nV): last=%i rms=%i min=%i max=%i   current=%i mA\r\n", voltage, rms, voltage_min, voltage_max, current);
			write(text);

			uint64_t result = voltage_min;
			result <<= 32;
			result |= voltage_max;
			intChar = result;

			result = rms;
			result <<= 32;
			result |= current;
			intChar2 = result;


#ifdef BINARY_LED
			bin_counter++;
			if (bin_counter % 2) {
				NRF51_GPIO_OUTSET = 1 << PIN_LED; // pin high, led goes on
			} else {
				NRF51_GPIO_OUTCLR = 1 << PIN_LED; // pin low, led goes off
			}
#endif


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
	// set scanning option
	localizationService.createCharacteristic<uint8_t>()
		.setUUID(UUID(localizationService.getUUID(), 0x123))
		.setName("number")
		.setDefaultValue(255)
		.setWritable(true)
		.onWrite([&](const uint8_t & value) -> void {
			int nr = value;
			switch(nr) {
			case 0: {
				const char* command = "Crown: start scanning\r\n";
				write(command);
				stack.startScanning();
			}
			break;
			case 1: {
				const char* command = "Crown: stop scanning\r\n";
				write(command);
				stack.stopScanning();
			}
			break;
			}
			
		});

	// set threshold level
	localizationService.createCharacteristic<uint8_t>()
		.setUUID(UUID(localizationService.getUUID(), 0x122))
		.setName("number")
		.setDefaultValue(0)
		.setWritable(true)
		.onWrite([&](const uint8_t & value) -> void {
			nrf_pwm_set_value(0, value);
//			nrf_pwm_set_value(1, value);
//			nrf_pwm_set_value(2, value);
			char text[64];
			sprintf(text, "set personal_threshold_value to %i \r\n", value);
			//write("set personal_threshold_value\r\n");
			write(text);
			personal_threshold_level = value;
		});

	// Begin sending advertising packets over the air.  Again, may want to trigger this from a button press to save power.
	stack.startAdvertising();

	// Init pwm
	nrf_pwm_config_t pwm_config = PWM_DEFAULT_CONFIG;
	pwm_config.mode             = PWM_MODE_LED_100;
	pwm_config.num_channels     = 2;
	pwm_config.gpio_num[0]      = PIN_LED;
	pwm_config.gpio_num[1]      = 9;
//	pwm_config.gpio_num[2]      = 10;
	nrf_pwm_init(&pwm_config);

//	// Start with the power off
//	nrf_gpio_pin_write(PIN_LED, 0);
//	nrf_pwm_set_value(0, 0);


	while(1) {
		// Deliver events from the Bluetooth stack to the callbacks defined above.
		//		analogWrite(PIN_LED, 50);
		stack.loop();
	}
}
