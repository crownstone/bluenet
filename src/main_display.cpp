/*************************************************************************************************
* BLE EPaper Display.
**************************************************************************************************/

#include <limits>


#include "BluetoothLE.h"
#include "S5813A.h"
#include "EPD_GFX.h"
#include "Font.h"
#include "Display.h"

#include "pinconfig.h"

#include "Midi.h"
#include "EPD_Display.h"



#include "random.h"

#include "ble_error.h"

extern "C"{
// TODO load these dynamically....
#include "fonts/MyriadWebPro_Bold_12.h"
#include "fonts/MyriadWebPro_12.h"
#include "fonts/MyriadWebPro_Bold_46.h"
#include "fonts/MyriadWebPro_28.h"
#include "fonts/MyriadWebPro_Bold_28.h"

}



extern "C" {
extern unsigned long _stext;
extern unsigned long _etext;
extern unsigned long _sdata;
extern unsigned long _edata;
extern unsigned long _sbss;
extern unsigned long _ebss;
extern unsigned long _estack;

}
using namespace BLEpp;

void terminator() throw ();

volatile uint8_t button = 0;
extern "C" {
void GPIOTE_IRQHandler() {
    if (NRF51_GPIOTE_IN_2) {
        if (!(NRF51_GPIO_IN & (1 << Pin_SW2))) {
            button = 1;
        } else {
            button = 2;
        }
        NRF51_GPIOTE_IN_2 = 0;
    } else if ( NRF51_GPIOTE_PORT ) {
        NRF51_GPIOTE_PORT = 0;
    }
    sd_nvic_ClearPendingIRQ(GPIOTE_IRQn);
}
}

static const uint16_t fastSlaveLatency = 0;
static const uint16_t slowSlaveLatency = 10;

int main(void) {

    //std::set_terminate(terminator);

    //try {

    /// STARTUP ////////////////////////////////////////////////////////////////////////////////////////////////////////


    // turn on all the power pins early.
    setup_power();

    // set up LED
    NRF51_GPIO_DIRSET = 1 << Pin_LED;
    NRF51_GPIO_OUTSET = 1 << Pin_LED;

    // pull microsd card CS line high so it doesn't engage.  this doesn't help.
    // NRF51_GPIO_PIN_CNF(Pin_uSD_CS) = NRF51_GPIO_PIN_CNF_INPUT | NRF51_GPIO_PIN_CNF_PULL_PULLUP;

    PoolImpl<255> pool;

    Nrf51822BluetoothStack stack(pool);
    stack.setDeviceName("wiSticky")
         .setTxPowerLevel(0)
         .setAppearance(BLE_APPEARANCE_GENERIC_DISPLAY) // controls how device appears in GUI.  elided for now to save space in advertising data.
         .init();  // start up the softdevice early because we need it's functions to configure devices it ultimately controls.

    // start the Real Time Counter (RTC) -- used for delay() etc. this needs to happen after the BLE stack is inited.
    // TODO should the stack do this automatically?  or maybe it should happen in
    tick_init();

    stack.setMinConnectionInterval(16)
         .setMaxConnectionInterval(32)
         .setConnectionSupervisionTimeout(400)
         .setSlaveLatency(slowSlaveLatency)
         .setAdvertisingInterval(1600)
         .setAdvertisingTimeoutSeconds(0);

    volatile bool led_override = false;
    stack.onRadioNotificationInterrupt(800, [&](bool radio_active){
        // NB: this executes in interrupt context.
        if (!led_override) {
            if (radio_active) {
                NRF51_GPIO_OUTSET = 1 << Pin_LED;
            } else {
                NRF51_GPIO_OUTCLR = 1 << Pin_LED;
            }
        }
    });

    // TODO: where should this go?
    NRF51_GPIO_PIN_CNF(Pin_EPD_CS) |= NRF51_GPIO_PIN_CNF_PULL_PULLUP;

    NRF51_GPIO_PIN_CNF(Pin_SW2) = NRF51_GPIO_PIN_CNF_SENSE_LOW;

    // start up the SPI device used for communicating with the EPaper display.
    setupSPI(Pin_MISO, Pin_MOSI, Pin_Sclk);

    // define the E-Paper display (EPD)
    EPD_Class EPD(EPD_SIZE, Pin_PANEL_ON, Pin_BORDER, Pin_DISCHARGE, Pin_PWM, Pin_RESET, Pin_BUSY, Pin_EPD_CS, SPI);

    static const coord_t h = EPD.height();
    static const coord_t w = EPD.width();

    // This is the temperature of the display in degrees C.  Higher temperatures result
    // in shorter update times, but more potential for ghosting.
    // Eventually we'll want to periodically measure and and adjust this.
    // Setting this to a lower value (like -10) can help eliminate ghosting,
    // or can be used to help repair a display that's been stuck in a bad state,
    // at the expensive of very long update cycles.
    EPD.setFactor(30);

    EPD_Display display(&pool, EPD);

    /// FONTS ////////////////////////////////////////////////////////////////////////////////////////////////////////
    Font myriadBold46(MyriadWebPro_Bold_46_pfo, MyriadWebPro_Bold_46_pfo + MyriadWebPro_Bold_46_pfo_size);
    Font myriad18(MyriadWebPro_28_pfo, MyriadWebPro_28_pfo + MyriadWebPro_28_pfo_size);
    Font myriadBold18(MyriadWebPro_Bold_28_pfo, MyriadWebPro_Bold_28_pfo + MyriadWebPro_Bold_28_pfo_size);
    display.setFont(&myriad18);

    // show an initialization message.
    display.clear();
    display.fillCheckerboard(0, 0, w, h, 1);
    display.drawString(20, 10, "Welcome to", myriadBold18, 0);
    display.drawString(40, 60, "wiSticky!", myriadBold46, 0);
    display.display();

    delay(1000);

    volatile int32_t count = 0;

//    for(uint32_t* addr = (uint32_t*)0x20002000; addr < (uint32_t*)0x20004000; ++addr) {
//        if ((*addr) == 0xdeadbeef) {
//            count++;
//        }
//    }


    /// BLUETOOTH SERVICES ///////////////////////////////////////////////////////////////////////////////////////////

//    BatteryService batteryService;
//
//    batteryService.setBatteryLevelHandler([&]()->uint8_t {
//        return 12; // TODO analogRead(batteryVoltagePin) or somesuch.
//    });
//    stack.addService(&batteryService); // can we get rid of & somehow?  have an override that takes Service& ?

    (uint32_t)count;

    Service& displayService = stack.createService()
        .setUUID(UUID("20CC0123-B57E-4365-9F35-31C9227D4C4B"));
        //.setName("Display Service");
    displayService.createCharacteristic<string>()
        .setUUID(UUID(displayService.getUUID(), 0x124))
        .setName("text")
        .setDefaultValue("")
        .setWritable(true)
        .onWrite([&](const string& value) -> void {
            display.clear();
            display.drawString(20, 10, value, myriadBold18, 1);
            display.display();
        });

//    displayService.createCharacteristic<uint32_t>()
//            .setUUID(UUID(displayService.getUUID(), 0x126))
//            .setName("height")
//            .setDefaultValue(h)
//            ;
//    displayService.createCharacteristic<uint32_t>()
//            .setUUID(UUID(displayService.getUUID(), 0x127))
//            .setName("width")
//            .setDefaultValue(w)
//            ;

    Characteristic<float>& drawTime = displayService.createCharacteristic<float>()
            .setUUID(UUID(displayService.getUUID(), 0x128))
            .setName("drawTime")
            .setDefaultValue(0)
            ;
//
//    Characteristic<uint32_t>& outOfOrder = displayService.createCharacteristic<uint32_t>()
//            .setUUID(UUID(displayService.getUUID(), 0x129))
//            .setName("outOfOrder")
//            .setDefaultValue(0)
//            ;

    uint8_t val[]= {1,2,3};

    struct Bob {
        Display& display;
        Characteristic<float>& drawTime;
        uint32_t start = 0;
        Bob(Display& dd, Characteristic<float>& dt) : display(dd), drawTime(dt) { }
    };
    Bob b(display, drawTime);

    CharacteristicValue dflt(2, val);
   // uint8_t lastWrite = 0;
//    volatile uint8_t writeCount = 0;
//    volatile uint8_t updateCount = 0;
    displayService.createCharacteristic<CharacteristicValue>()
            .setUUID(UUID(displayService.getUUID(), 0x125))
            .setName("bitmap")
            .setDefaultValue(dflt)
            .setWritable(true)
            .onWrite([&](const CharacteristicValue& value) {

        uint8_t command = *(uint8_t*)(value.data);
        uint8_t seq = *(uint8_t*)(value.data+1);

//        if (seq != lastWrite + 1) {
//            outOfOrder += 1u;
//        }
//        lastWrite = seq;
       // uint8_t header_length = 6;

        if (command == 'p') { // pixels

//            uint8_t len = value.length;
//
//            uint16_t x = *(uint16_t*)(value.data+2);
//            uint16_t y = *(uint16_t*)(value.data+4);
//            for(int i = header_length; i < len; ++i) {
//                 uint8_t byte = value.data[i];
//                 for(uint8_t j = 0; j < 8; ++j) {
//                    boolean bit = byte & 1 << j;
//                    gfx.drawPixel(x++, y, bit ? 1 : 0);
//                    if (x == w) {
//                        x = 0;
//                        y++;
//                    }
//                }
//            }
        } else if (command == 'P') { // run length encoded pixels

            if (b.start == 0) b.display.clear();

            if (seq == 0) {
                b.start = NRF51_RTC1_COUNTER;
            }

            b.display.drawImage(RLEImageMessage(value.data, value.length));


        } else if (command == 'd') { // display

            b.display.display();
            b.display.clear();
            uint32_t end = NRF51_RTC1_COUNTER;

            b.drawTime = (float)(end < b.start ? std::numeric_limits<uint32_t>::max() - b.start + end : end - b.start) / (float)(32768 / NRF51_RTC1_PRESCALER + 1);
            //updateCount++;
        } else {
            __asm("BKPT");
            while (1) {}
        }

        //writes += 1u;
       // writeCount++;

    });



    auto startAdvertising = [&]{

        // turn on the LED, since we're advertising again.
        //NRF51_GPIO_OUTSET = 1 << Pin_LED;

        // start sending out advertisements over Bluetooth.  This continues for 90 seconds currently.
        stack.startAdvertising();

        display.clear();
        display.fillCheckerboard(0, 0, w, h, 1);
        display.drawString(20, 10, "Advertising...", myriadBold18, 0);
        display.display();
    };

    stack.onConnect([&](uint16_t conn_handle) {
               display.clear();
               display.fillCheckerboard(0, 0, w, h, 1);
               display.drawString(20,10, "Connected.", myriadBold18, 0);
               // display.drawString(80,50, addr, myriadBold18);
                display.display();


                // turn off the LED, since we're all started advertising.
                digitalWrite(Pin_LED, 0);
                //updateCount = 0;
            })
         .onDisconnect([&](uint16_t conn_handle) {
                display.clear();
                display.display();
                delay(2000);
                startAdvertising();
            });

    //stack.onTimeout()

    count = 0;

    for(uint32_t* addr = (uint32_t*)0x20002000; addr < (uint32_t*)0x20004000; ++addr) {
        if ((*addr) == 0xdeadbeef) {
            count++;
        }
    }

    startAdvertising();

    volatile uint32_t freemem = count;
    (uint32_t)freemem;

    sd_power_mode_set(NRF_POWER_MODE_LOWPWR);
    bool button_down = false;
    uint32_t button_down_time = 0;

    enable_irq(NRF51_GPIOTE_IRQn, IRQ_APP_PRIORITY_LOW);

    NRF51_GPIOTE_INTENSET = NRF51_EVENT_INTERRUPT_N(NRF51_GPIOTE, NRF51_GPIOTE_PORT );
    NRF51_GPIOTE_PORT = 0;
    sd_nvic_ClearPendingIRQ(GPIOTE_IRQn);

    while(1) {

        uint32_t now = NRF51_RTC1_COUNTER;

        if (!(NRF51_GPIO_IN & (1 << Pin_SW2))) {
            if (!button_down) {
                button_down = true;
                button_down_time = now;
                led_override = true;
                NRF51_GPIO_OUTSET = 1 << Pin_LED;
            }
        } else {
            if (button_down) {
                button_down = false;
                led_override = false;

                uint32_t diff = ((now > button_down_time) ? (now - button_down_time) : (NRF51_RTC1_COUNTER_MAX - button_down_time + now));
                if (diff > 1000) {
                    stack.stopAdvertising();
                    display.clear();
                    display.display();
                    stack.shutdown();
                    NRF51_GPIO_OUTCLR = 1 << Pin_LED;
                    NRF51_GPIO_PIN_CNF(Pin_SW2) = NRF51_GPIO_PIN_CNF_PULL_PULLUP | NRF51_GPIO_PIN_CNF_OUTPUT;
                    NRF51_GPIO_OUTSET = 1 << Pin_SW2;
                    delay(5000);

                    NRF51_GPIO_OUTCLR = 1 << Pin_PowerOn_2;
                }
                NRF51_GPIO_OUTCLR = 1 << Pin_LED;
            }
        }
        button = 0;
        NRF51_GPIOTE_PORT = 0;
        sd_nvic_ClearPendingIRQ(GPIOTE_IRQn);

        // deliver events from the Bluetooth stack to the callbacks defined above.
        stack.loop();
    }
/*

} catch(exception& e) {

    __asm("BKPT");

} catch (...) {
    __asm("BKPT");
}
*/


}



