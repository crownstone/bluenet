
#include <string>
#include <limits>
#include <algorithm>
#include <random>
#include "stdio.h"

#include "EPD_Display.h"
#include "pinconfig.h"
#include "ble_error.h"


extern "C"{
// TODO load these dynamically....
#include "fonts/MyriadWebPro_Bold_12.h"
#include "fonts/MyriadWebPro_12.h"
#include "fonts/MyriadWebPro_Bold_46.h"
#include "fonts/MyriadWebPro_Bold_64.h"
#include "fonts/MyriadWebPro_28.h"
#include "fonts/MyriadWebPro_Bold_28.h"
#include "fonts/MyriadWebPro_Bold_32.h"



#include "nrf_gpio.h"
}


#include "images/test_image_1.h"

#include "images/test_image_2.h"

#include  "HuffmanImage.h"
#include "images/huff_test_image_1.h"


void terminator() throw ();

int main(void) {

    //std::set_terminate(terminator);

    // we're not using exceptions currently because they add some pretty significant size to the executable.
    // But it might be wise in a real production app.
    //try {

    // printf("hello, world.");  // semihosted debug output is not yet working with segger jlink.  see http://www.segger2.com/index.php?page=Thread&threadID=1599

    // turn on all the power pins early.
    setup_power();

    // turn on LED during startup.
    NRF51_GPIO_DIRSET = 1 << Pin_LED;
//    NRF51_GPIO_OUTSET = 1 << Pin_LED;

    // start the Real Time Counter (RTC) -- used for delay() etc.
    tick_init();

    // This is the memory pool.  It creates a uniform set of memory blocks that we can use for
    // dynamic allocation without the worry of fragmentation -- because every block is the same
    // size, there's no way our request for memory can fail unless we're completely out.
    // Pooled objects implement the new operator and either fit entirely within one block,
    // or manage their own links to additional blocks.
    PoolImpl<255> pool;

    // Turn on the pullup resistor for the EPD chip select.  Needed to ensure the voltage doesn't "float."
    // TODO: where should this go?
    NRF51_GPIO_PIN_CNF(Pin_EPD_CS) |= NRF51_GPIO_PIN_CNF_PULL_PULLUP;

    // start up the SPI device used for communicating with the EPaper display.
    setupSPI(Pin_MISO, Pin_MOSI, Pin_Sclk);

    // Define the E-Paper Display (EPD).  This is vendor code from repaper.org, slightly modified to
    // support callbacks with lambdas.
    EPD_Class EPD(EPD_SIZE, Pin_PANEL_ON, Pin_BORDER, Pin_DISCHARGE, Pin_PWM, Pin_RESET, Pin_BUSY, Pin_EPD_CS, SPI);

    coord_t h = EPD.height();
    coord_t w = EPD.width();

    // This is the Display implementation for the EPD.  It contains two DisplayLists, between which
    // we alternate to provide the ability to erase the previous image as required by the EPD.
    EPD_Display display(&pool, EPD);

    string hello = "Hello, world...";

    /// FONTS ////////////////////////////////////////////////////////////////////////////////////////////////////////
    // these fonts get embedded in the executable at significant size cost.  may want to load from SD card.
    Font myriadBold46(MyriadWebPro_Bold_46_pfo, MyriadWebPro_Bold_46_pfo + MyriadWebPro_Bold_46_pfo_size);
    Font myriadBold64(MyriadWebPro_Bold_64_pfo, MyriadWebPro_Bold_64_pfo + MyriadWebPro_Bold_64_pfo_size);
    Font myriad18(MyriadWebPro_28_pfo, MyriadWebPro_28_pfo + MyriadWebPro_28_pfo_size);
    Font myriadBold32(MyriadWebPro_Bold_32_pfo, MyriadWebPro_Bold_32_pfo + MyriadWebPro_Bold_32_pfo_size);
    Font myriadBold18(MyriadWebPro_Bold_28_pfo, MyriadWebPro_Bold_28_pfo + MyriadWebPro_Bold_28_pfo_size);

    #define NUM_FONTS 5
    Font* fonts[] = {&myriad18, &myriadBold46, &myriadBold64, &myriadBold32, &myriadBold18};

    // compute text bounds.
    rect meas[NUM_FONTS];
    std::transform (fonts, fonts+NUM_FONTS, meas, [&](Font* f){
        return f->measureString(hello);
    });

    /// RANDOMNESS ////////////////////////////////////////////////////////////////////////////////////////////////////////

    // TODO use the hardware RNG.
    std::default_random_engine random(23493459);
    std::uniform_int_distribution<uint8_t> fonti(0, NUM_FONTS-1);
    std::uniform_int_distribution<uint8_t> factori(5, 30);



    // TODO something is wrong with the encoded image data here.
//    RLEImageMessageFactory factory(display.drawImage());
//    factory.add(test_image_1, test_image_1_len);
//    display.display();
//
//    delay(1000);

//    display.clear();

    HuffmanImage* ri = dynamic_cast<HuffmanImage*>(display.active().peekGroup().find_first([](DisplayObject* d) {
        return dynamic_cast<HuffmanImage*>(d) != 0;
    }));
    if (!ri) {
        ri = new (display.active().getPool()) HuffmanImage(&display.active().peekGroup(), 0, 0, display.active().width(), display.active().height());
        display.active().appendChild(ri);
    }
    HuffmanImage& huffmanImage = *ri;

    HuffmanCodebookMessageFactory huffmanCodeMessageFactory(huffmanImage);
    HuffmanImageMessageFactory huffmanImageMessageFactory(huffmanImage);

    huffmanCodeMessageFactory.add(huff_test_book_1,huff_test_book_1_len);
    huffmanImageMessageFactory.add(huff_test_image_1,huff_test_image_1_len);

    display.display();
//    display.clear();

    delay(1000);
//
//    display.display();

//    ri->~HuffmanImage();

    int i = 0;
    while(1) {


//        // This is the temperature of the display in degrees C.  Higher temperatures result
//        // in shorter update times, but more potential for ghosting.  In order to demonstrate that
//        // effect, we make up a random temperature and show that in the bottom right of the screen.
//        // In a real application we'd use the measured physical temperature of the device.
//        int32_t factor = factori(random);
//        EPD.setFactor(factor);
//
//        display.clear();
//
//        if (i%2 == 0) display.fillCheckerboard(0, 0, w, h, 1);
//
//        volatile size_t fi = fonti(random);
//        Font& f = *fonts[fi];
//        rect& m = meas[fi];
//
//        std::uniform_int_distribution<coord_t> randw(std::min(0, w-m.width), std::max(0, w-m.width));
//        std::uniform_int_distribution<coord_t> randh(std::min(0, h-m.height), std::max(0,h-m.height));
//
//        volatile coord_t xx = randw(random);
//        volatile coord_t yy = randh(random);
//
//        char buf[15];
//        sprintf(buf, "%u", factor);
//        string s(buf);
//        rect sm = fonts[0]->measureString(s);
//
//        display.drawString(xx, yy, hello, f,  i % 4 > 0);
//
//        if (i % 5 == 0) {
//            // Overlay a white checkerboard.  This effect will be used when the display data has exceeded it's TTL.
//            display.fillCheckerboard(0, 0, w, h-sm.height, 0);
//            display.fillCheckerboard(0, h-sm.height, w-sm.width, sm.height, 0);
//        }
//
//        display.drawString(w-sm.width, h-sm.height, s, *fonts[0],  i % 4 > 0);

        // render it to the physical device.
//        display.display();

        i++;

    }
}