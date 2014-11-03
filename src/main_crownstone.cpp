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

extern "C" {
	#include "ble_advdata_parser.h"
	#include "nrf_delay.h"
}

#include "nordic_common.h"
#include "nRF51822.h"

#include <stdbool.h>
#include <stdint.h>
#include <cstring>

#include "uart.h"
#include "log.h"

#include "IndoorLocalisationService.h"
#include "TemperatureService.h"

using namespace BLEpp;

//#ifndef BOARD_NRF6310
//#define BOARD_NRF6310
//#endif

// on the RFduino
#define PIN_RED              2                   // this is GPIO 2 (bottom pin)
#define PIN_GREEN            3                   // this is GPIO 3 (second pin)
#define PIN_BLUE             4                   // this is GPIO 4 (third pin)

#ifdef BOARD_NRF6310
	extern "C" {
	#include "boards.h"
	}
	#define PIN_LED          LED_0               // this is P1.0
#else
	#define PIN_LED          0                   // this is GPIO 0
#endif

// this is the switch on the 220V plug!
#define BINARY_LED

// An RGB led as with the rfduino requires a sine wave, and thus a PWM signal
//#define RGB_LED

//#define MOTOR_CONTROL

//#define IBEACON

//
#define INDOOR_SERVICE
#define TEMPERATURE_SERVICE


// the characteristics that need to be included
//#define NUMBER_CHARAC
#define CONTROL_CHARAC

#define PIN_MOTOR            6                   // this is GPIO 6 (fifth pin)

struct peripheral_t {
//	uint8_t addr[BLE_GAP_ADDR_LEN];
	char addrs[28];
	uint16_t occurences;
	int8_t rssi;
};

//static std::vector<peripheral_t> _history;
#define HISTORY_SIZE 10
peripheral_t _history[HISTORY_SIZE];
uint8_t freeIdx = 0;

/** Example that sets up the Bluetooth stack with two characteristics:
 *   one textual characteristic available for write and one integer characteristic for read.
 * See documentation for a detailed discussion of what's going on here.
 **/
int main() {
	uart_init(UART_BAUD_38K4);

	LOG_INFO("Welcome at the nRF51822 code for meshing.");


	int personal_threshold_level;
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
	stack.setDeviceName(std::string("Crown"))
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
//	stack.onRadioNotificationInterrupt(800, [&](bool radio_active){
//			// NB: this executes in interrupt context, so make any global variables volatile.
//			if (radio_active) {
//			// radio is about to turn on.  Turn on LED.
//			// NRF51_GPIO_OUTSET = 1 << Pin_LED;
//			} else {
//			// radio has turned off.  Turn off LED.
//			// NRF51_GPIO_OUTCLR = 1 << Pin_LED;
//			}
//			});

	//uint32_t err_code = NRF_SUCCESS;

	stack.onConnect([&](uint16_t conn_handle) {
		LOG_INFO("onConnect...");
			// A remote central has connected to us.  do something.
			// TODO this signature needs to change
			//NRF51_GPIO_OUTSET = 1 << PIN_LED;
			// first stop, see https://devzone.nordicsemi.com/index.php/about-rssi-of-ble
			// be neater about it... we do not need to stop, only after a disconnect we do...
			sd_ble_gap_rssi_stop(conn_handle);
			sd_ble_gap_rssi_start(conn_handle);
		})
		.onDisconnect([&](uint16_t conn_handle) {
			LOG_INFO("onDisconnect...");
			// A remote central has disconnected from us.  do something.
			//NRF51_GPIO_OUTCLR = 1 << PIN_LED;

			// Of course this is not nice, but dirty! We immediately start advertising automatically after being
			// disconnected. But for now this will be the default behaviour.

			stack.stopScanning();

#ifdef IBEACON
			stack.startIBeacon();
#else
			stack.startAdvertising();
#endif
		})
		.onAdvertisement([&](ble_gap_evt_adv_report_t* p_adv_report) {
			//data_t adv_data;
			//data_t type_data;

			uint8_t * adrs_ptr = (uint8_t*) &(p_adv_report->peer_addr.addr);
			char addrs[28];
			sprintf(addrs, "[%02X %02X %02X %02X %02X %02X]", adrs_ptr[5],
					adrs_ptr[4], adrs_ptr[3], adrs_ptr[2], adrs_ptr[1],
					adrs_ptr[0]);

//			LOG_INFO("Advertisement from: %s, rssi: %d", addrs, p_adv_report->rssi);

//			std::vector<peripheral_t>::iterator it = _history.begin();
			uint16_t occ;
			bool found = false;
//			LOG_INFO("addrs: %s", addrs);
			for (int i = 0; i < freeIdx; ++i) {
//				LOG_INFO("_history[%d]: %s", i, _history[i].addrs);
				if (strcmp(addrs, _history[i].addrs) == 0) {
//					LOG_INFO("found");
					_history[i].occurences++;
					occ = _history[i].occurences;
					int avg_rssi = ((occ-1)*_history[i].rssi + p_adv_report->rssi)/occ;
//					_history[i].rssi = p_adv_report->rssi;
					_history[i].rssi = avg_rssi;
					found = true;
				}
			}
			if (!found) {
				uint8_t idx;
				if (freeIdx >= HISTORY_SIZE) {
					// history full, throw out item with lowest occurence
					uint16_t minOcc = UINT16_MAX;
					for (int i = 0; i < HISTORY_SIZE; ++i) {
						if (_history[i].occurences < minOcc) {
							minOcc = _history[i].occurences;
							idx = i;
						}
					}
				} else {
					idx = freeIdx++;
					peripheral_t peripheral;
					_history[idx] = peripheral;
				}

				LOG_INFO("NEW:\tAdvertisement from: %s, rssi: %d", addrs, p_adv_report->rssi);
//				peripheral.addrs = addrs;
				strcpy(_history[idx].addrs, addrs);
				_history[idx].occurences = 1;
				_history[idx].rssi = p_adv_report->rssi;
			} else {
//				LOG_INFO("\tAdvertisement from: %s, rssi: %d, occ: %d", addrs, p_adv_report->rssi, occ);
			}

//			ble_advdata_t advdata;
//			memset(&advdata, 0, sizeof(advdata));
//
//
//			uint8_t len = p_adv_report->dlen;
//			unsigned char* p_name;
//			if (NRF_SUCCESS == ble_advdata_parser_field_find(BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, p_adv_report->data, &len, &p_name)) {
//
//
//				LOG_INFO("len: %d", len);
//				char name[len+1];
//				strncpy(name, (const char*)p_name, len);
//				name[len] = '\0';
//
//				LOG_INFO("Advertisement from: %s %s, rssi: %d", name, addrs, p_adv_report->rssi);
//
//			} else {
//				LOG_INFO("Advertisement from: %s, rssi: %d", addrs, p_adv_report->rssi);
//			}

			/*
						 * 			uint8_t len = p_adv_report->dlen;
						unsigned char* p_name;
						char* _name = "";
						if (NRF_SUCCESS == ble_advdata_parser_field_find(BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, p_adv_report->data, &len, &p_name)) {


							LOG_INFO("len: %d", len);
			//				char name[len+1];
							_name = new char[len+1];
			//				strncpy(_name, (const unsigned char*)p_name, len);
							memcpy(_name, p_name, len);
							_name[len] = '\0';

			//				char* name = "OK";

						} else {
				//			LOG_INFO("no name found");
			//				LOG_INFO("Advertisement from: %s, rssi: %d", addrs, p_adv_report->rssi);
							_name = "";
						}
						LOG_INFO("Advertisement from: %s %s, rssi: %d", _name, addrs, p_adv_report->rssi);
						 *
			*/

			// Initialize advertisement report for parsing.
			//adv_data.p_data = (uint8_t *)p_gap_evt->params.adv_report.data;
			//adv_data.data_len = p_gap_evt->params.adv_report.dlen;



		});

	//Service& generalService = stack.createService();
	//Service& batteryService = stack.createBatteryService();


#ifdef INDOOR_SERVICE
	// Now, build up the services and characteristics.
	Service& localizationService = IndoorLocalizationService::createService(stack);


#ifdef NUMBER_CHARAC
	// Create a characteristic of type uint8_t (unsigned one byte integer).
	// This characteristic is by default read-only (for the user)
	// Note that in the next characteristic this variable intChar is set!
	Characteristic<uint8_t>& intChar = localizationService.createCharacteristic<uint8_t>()
		.setUUID(UUID(localizationService.getUUID(), 0x125))  // based off the UUID of the service.
		.setName("number");
#endif // _NUMBER_CHARAC

#ifdef CONTROL_CHARAC
	localizationService.createCharacteristic<uint8_t>()
		.setUUID(UUID(localizationService.getUUID(), 0x124))
		//		.setName("number input")
		.setName("led")
		.setDefaultValue(255)
		.setWritable(true)
		.onWrite([&](const uint8_t& value) -> void {
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
//			std::string msg = std::string("Received message: ") + value;
//			uart.println(msg.c_str());
			LOG_INFO("Received message: %d", value);
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

		});
#endif // _CONTROL_CHARAC

	// set scanning option
	localizationService.createCharacteristic<uint8_t>()
		.setUUID(UUID(localizationService.getUUID(), 0x123))
		.setName("scan")
		.setDefaultValue(255)
		.setWritable(true)
		.onWrite([&](const uint8_t & value) -> void {
			switch(value) {
			case 0: {
				LOG_INFO("Crown: start scanning");
				stack.startScanning();
				break;
			}
			case 1: {
				LOG_INFO("Crown: stop scanning");
				stack.stopScanning();
				break;
			}
		}

	});

	// get scan result
	localizationService.createCharacteristic<uint8_t>()
		.setUUID(UUID(localizationService.getUUID(), 0x121))
		.setName("devices")
		.setDefaultValue(255)
		.setWritable(true)
		.onWrite([&](const uint8_t & value) -> void {
//			personal_threshold_level = value;
//			LOG_INFO("Setting personal threshold level to: %d", value);

			LOG_INFO("##################################################");
			LOG_INFO("### listing detected peripherals #################");
			LOG_INFO("##################################################");
			for (int i = 0; i < freeIdx; ++i) {
				LOG_INFO("%s\trssi: %d\tocc: %d", _history[i].addrs, _history[i].rssi, _history[i].occurences);
			}
			LOG_INFO("##################################################");

		});

	// set threshold level
	localizationService.createCharacteristic<uint8_t>()
		.setUUID(UUID(localizationService.getUUID(), 0x122))
		.setName("threshold")
		.setDefaultValue(255)
		.setWritable(true)
		.onWrite([&](const uint8_t & value) -> void {
			personal_threshold_level = value;
			LOG_INFO("Setting personal threshold level to: %d", value);
		});

#endif /* LOCALISATION_SERVICE */

#ifdef TEMPERATURE_SERVICE
	//	 get temperature value
	TemperatureService& temperatureService = TemperatureService::createService(stack);
//	temperatureService.start();
#endif /* TEMPERATURE_SERVICE */

	// Begin sending advertising packets over the air.
#ifdef IBEACON
	stack.startIBeacon();
#else
	stack.startAdvertising();
#endif

	LOG_INFO("running...");
	while(1) {
		// Deliver events from the Bluetooth stack to the callbacks defined above.
		//		analogWrite(PIN_LED, 50);
		stack.loop();

#ifdef TEMPERATURE_SERVICE
		// [31.10.14] correction, this only happens without optimization -Os !!!
		// [31.10.14] this seems to interfere with the scanning / advertisement data
		// coming in so if the temperature is read at the same time as scanning is
		// performed the whole thing crashes after some time.
		// NOTE: this probably needs to be done with other functionality too that has
		// to run at the same time as the scanning
//		if (!stack.isScanning() && stack.connected()) {
//			LOG_INFO("temp.loop()");
			temperatureService.loop();
//		}
#endif

		nrf_delay_ms(50);
	}
}
