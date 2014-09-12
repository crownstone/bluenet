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

#include "nordic_common.h"
#include "nRF51822.h"

#include <stdbool.h>
#include <stdint.h>

using namespace BLEpp;

#define PIN_LED              0                   // GPIO 0, or AREF (near "factory" on rfduino)

// enable the led
#define BINARY_LED

int main() {
#ifdef BINARY_LED
	NRF51_GPIO_DIRSET = 1 << PIN_LED; // set pins to output
	NRF51_GPIO_OUTCLR = 1 << PIN_LED; // pin low, led goes off
	uint32_t counter = 0;
#endif
	// Memory pool of 30 blocks of 30 bytes each, this already crashes the thing...
	PoolImpl<30> pool;

	// Set up the bluetooth stack that controls the hardware.
	Nrf51822BluetoothStack stack(pool);

	// Set advertising parameters such as the device name and appearance.  These values will
	stack.setDeviceName("Fit")
		// controls how device appears in GUI.
		.setAppearance(BLE_APPEARANCE_GENERIC_TAG);

	stack.setTxPowerLevel(-4)
		.setMinConnectionInterval(16)
		.setMaxConnectionInterval(32)
		.setConnectionSupervisionTimeout(400)
		.setSlaveLatency(10)
		.setAdvertisingInterval(1600)
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

	// Now, build up the services and characteristics.
	Service& service = stack.createIndoorLocalizationService();

	// Create a characteristic of type uint8_t (unsigned one byte integer).
	// This characteristic is by default read-only and
	Characteristic<uint8_t>& intChar = service.createCharacteristic<uint8_t>()
		.setUUID(UUID(service.getUUID(), 0x125))  // based off the UUID of the service.
		.setName("number");

	service.createCharacteristic<string>()
		.setUUID(UUID(service.getUUID(), 0x124))
		.setName("text")
		.setDefaultValue("")
		.setWritable(true)
		.onWrite([&](const string& value) -> void {
				//.onWrite([&](const uint8_t& value) -> void {
				// set the value of the "number" characteristic to the value written to the text characteristic.
				//int nr = value;
#ifdef BINARY_LED
			counter++;
			if (counter % 2) {
				NRF51_GPIO_OUTSET = 1 << PIN_LED; // pin high, led goes on
			} else {
				NRF51_GPIO_OUTCLR = 1 << PIN_LED; // pin low, led goes off
			}
#endif
		});

		// Begin sending advertising packets over the air.  Again, may want to trigger this from a button press to save power.
		stack.startAdvertising();
		while(1) {
			// Deliver events from the Bluetooth stack to the callbacks defined above.
			stack.loop();
		}
}
