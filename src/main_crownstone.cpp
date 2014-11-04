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

#include "nrf_adc.h"

#include <stdbool.h>
#include <stdint.h>
#include <cstring>
#include <cstdio>

#include "serial.h"

#include "Peripherals.h"

#include "IndoorLocalisationService.h"
#include "TemperatureService.h"

using namespace BLEpp;

/**********************************************************************************************************************
 * Enable the services you want to run on the device
 *********************************************************************************************************************/

#define INDOOR_SERVICE
//#define TEMPERATURE_SERVICE

/**********************************************************************************************************************
 * Main functionality
 *********************************************************************************************************************/

void welcome() {
	config_uart();
	log(INFO,"Welcome at the nRF51822 code for meshing.");
	log(INFO, "Compilation time: %s", COMPILATION_TIME);
}

void setName(Nrf51822BluetoothStack &stack) {
	char devicename[32];
	sprintf(devicename, "crow_%s", COMPILATION_TIME);
	stack.setDeviceName(std::string(devicename)) // max len = ble_gap_devname_max_len (31)
		// controls how device appears in gui.
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

int main() {
	welcome();

	// memory pool of 30 blocks of 30 bytes each, this already crashes the thing...
	PoolImpl<30> pool;

	// set up the bluetooth stack that controls the hardware.
	Nrf51822BluetoothStack stack(pool);

	// set advertising parameters such as the device name and appearance.  
	setName(stack);

	// configure parameters for the Bluetooth stack
	configure(stack);

	// start up the softdevice early because we need it's functions to configure devices it ultimately controls.
	// in particular we need it to set interrupt priorities.
	stack.init();

	stack.onConnect([&](uint16_t conn_handle) {
			log(INFO,"onConnect...");
			// todo this signature needs to change
			//NRF51_GPIO_OUTSET = 1 << PIN_LED;
			// first stop, see https://devzone.nordicsemi.com/index.php/about-rssi-of-ble
			// be neater about it... we do not need to stop, only after a disconnect we do...
			sd_ble_gap_rssi_stop(conn_handle);
			sd_ble_gap_rssi_start(conn_handle);
		})
		.onDisconnect([&](uint16_t conn_handle) {
			log(INFO,"onDisconnect...");
			//NRF51_GPIO_OUTCLR = 1 << PIN_LED;

			// of course this is not nice, but dirty! we immediately start advertising automatically after being
			// disconnected. but for now this will be the default behaviour.

			stack.stopScanning();

#ifdef ibeacon
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
			sprintf(addrs, "[%02x %02x %02x %02x %02x %02x]", adrs_ptr[5],
					adrs_ptr[4], adrs_ptr[3], adrs_ptr[2], adrs_ptr[1],
					adrs_ptr[0]);

//			log(INFO,"advertisement from: %s, rssi: %d", addrs, p_adv_report->rssi);

//			std::vector<peripheral_t>::iterator it = _history.begin();
			uint16_t occ;
			bool found = false;
//			log(INFO,"addrs: %s", addrs);
			for (int i = 0; i < freeidx; ++i) {
//				log(INFO,"_history[%d]: %s", i, _history[i].addrs);
				if (strcmp(addrs, _history[i].addrs) == 0) {
//					log(INFO,"found");
					_history[i].occurences++;
					occ = _history[i].occurences;
					int avg_rssi = ((occ-1)*_history[i].rssi + p_adv_report->rssi)/occ;
//					_history[i].rssi = p_adv_report->rssi;
					_history[i].rssi = avg_rssi;
					found = true;
				}
			}
			if (!found) {
				uint8_t idx = -1;
				if (freeidx >= history_size) {
					// history full, throw out item with lowest occurence
					uint16_t minocc = UINT16_MAX;
					for (int i = 0; i < history_size; ++i) {
						if (_history[i].occurences < minocc) {
							minocc = _history[i].occurences;
							idx = i;
						}
					}
				} else {
					idx = freeidx++;
					peripheral_t peripheral;
					_history[idx] = peripheral;
				}
				if (idx >= 0) {

					log(INFO,"new:\tadvertisement from: %s, rssi: %d", addrs, p_adv_report->rssi);
					//				peripheral.addrs = addrs;
					strcpy(_history[idx].addrs, addrs);
					_history[idx].occurences = 1;
					_history[idx].rssi = p_adv_report->rssi;
				}
			} else {
//				log(INFO,"\tadvertisement from: %s, rssi: %d, occ: %d", addrs, p_adv_report->rssi, occ);
			}
		});

	//service& generalservice = stack.createService();
	//service& batteryservice = stack.createbatteryservice();


#ifdef INDOOR_SERVICE
	// now, build up the services and characteristics.
	//Service& localizationService = 
	IndoorLocalizationService::createService(stack);
#endif

#ifdef TEMPERATURE_SERVICE
	//	 get temperature value
	TemperatureService& temperatureService = TemperatureService::createService(stack);
//	temperatureservice.start();
#endif /* temperature_service */

	// begin sending advertising packets over the air.
#ifdef IBEACON
	stack.startIBeacon();
#else
	stack.startAdvertising();
#endif

	log(INFO,"Running while loop..");

	while(1) {
		// deliver events from the bluetooth stack to the callbacks defined above.
		//		analogwrite(pin_led, 50);
		stack.loop();

#ifdef TEMPERATURE_SERVICE
		// [31.10.14] correction, this only happens without optimization -Os !!!
		// [31.10.14] this seems to interfere with the scanning / advertisement data
		// coming in so if the temperature is read at the same time as scanning is
		// performed the whole thing crashes after some time.
		// note: this probably needs to be done with other functionality too that has
		// to run at the same time as the scanning
//		if (!stack.isscanning() && stack.connected()) {
//			log(INFO,"temp.loop()");
			temperatureService.loop();
//		}
#endif
			nrf_delay_ms(50);
	}
}
