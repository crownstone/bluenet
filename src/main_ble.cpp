#include "Pool.h"
#include "BluetoothLE.h"
#include "ble_error.h"
#include "ble_stack_handler.h"
#include "nordic_common.h"
#include "ble_nrf6310_pins.h"

using namespace BLEpp;

/** Example that sets up the Bluetooth stack with two characteristics:
 *   one textual characteristic available for write and one integer characteristic for read.
 * See documentation for a detailed discussion of what's going on here.
 **/
int main() {

    // Memory pool of 30 blocks of 30 bytes each, this already crashes the thing...
    PoolImpl<30> pool;

    // Set up the bluetooth stack that controls the hardware.
    Nrf51822BluetoothStack stack(pool);

    // Set advertising parameters such as the device name and appearance.  These values will
    stack.setDeviceName("DoBotsIndoors")
         // controls how device appears in GUI.
         .setAppearance(BLE_APPEARANCE_GENERIC_DISPLAY);

   // .setUUID(UUID("20CC0123-B57E-4365-9F35-31C9227D4C4B"));

    // Set connection parameters.  These trade off responsiveness and range for battery life.  See Apple Bluetooth Accessory Guidelines for details.
    // You could omit these; there are reasonable defaults that support medium throughput and long battery life.
    //   interval is set from 20 ms to 40 ms
    //   slave latency is 10
    //   supervision timeout multiplier is 400
    stack.setTxPowerLevel(0)
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
	    
	    uint32_t err_code = NRF_SUCCESS;

	    // first stop, see https://devzone.nordicsemi.com/index.php/about-rssi-of-ble
	    // be neater about it... we do not need to stop, only after a disconnect we do...
	    err_code = sd_ble_gap_rssi_stop(conn_handle);

	    err_code = sd_ble_gap_rssi_start(conn_handle);

         })
         .onDisconnect([&](uint16_t conn_handle) {
            // A remote central has disconnected from us.  do something.
         });


    // Now, build up the services and characteristics.
    Service& service = stack.createIndoorLocalizationService();
    //Service& service = stack.createBatteryService();
/*
 * // Create a characteristic of type uint8_t (unsigned one byte integer).
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

        // set the value of the "number" characteristic to the value written to the text characteristic.
	int nr = atoi(value.c_str());
        intChar = nr;
    });
*/

    // Begin sending advertising packets over the air.  Again, may want to trigger this from a button press to save power.
    stack.startAdvertising();

    while(1) {
        // Deliver events from the Bluetooth stack to the callbacks defined above.
        stack.loop();
    }


}
