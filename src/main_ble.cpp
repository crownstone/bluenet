

#include "Pool.h"
#include "BluetoothLE.h"
#include "ble_error.h"

using namespace BLEpp;

/** Example that sets up the Bluetooth stack with two characteristics:
 *   one textual characteristic available for write and one integer characteristic for read.
* See documentation for a detailed discussion of what's going on here.
* */
int main() {

    // Memory pool of 30 blocks of 30 bytes each.
    PoolImpl<30> pool;

    // Set up the bluetooth stack that controls the hardware.
    Nrf51822BluetoothStack stack(pool);

    // Set advertising parameters such as the device name and appearance.  These values will
    stack.setDeviceName("BLEpp")
         // controls how device appears in GUI.
         .setAppearance(BLE_APPEARANCE_GENERIC_DISPLAY);


    // Set connection parameters.  These trade off responsiveness and range for battery life.  See Apple Bluetooth Accessory Guidelines for details.
    // You could omit these; there are reasonable defaults that support medium throughput and long battery life.
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

    stack.onConnect([&](uint16_t conn_handle) {
            // A remote central has connected to us.  do something.
            // TODO this signature needs to change
         })
         .onDisconnect([&](uint16_t conn_handle) {
            // A remote central has disconnected from us.  do something.
         });


    // Now, build up the services and characteristics.

    // Create a "service" that groups one or more characteristics:
    Service& service = stack.createService()
            .setUUID(UUID("20CC0123-7E02-4069-886B-D235C2B83BBE"));

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

        // set the value of the "number" characteristic to the value written to the text characteristic.
        intChar = atoi(value.c_str());
    });

    // Begin sending advertising packets over the air.  Again, may want to trigger this from a button press to save power.
    stack.startAdvertising();

    while(1) {
        // Deliver events from the Bluetooth stack to the callbacks defined above.
        stack.loop();
    }


}