#include "Display.h"
#include "EPD_Display.h"

void setupSPI(int Pin_MISO, int Pin_MOSI, int Pin_Sclk) {

    // start up the SPI device used for communicating with the EPaper display.
    SPI.setEnablePullResistors(true);
    SPI.setPinMiso(Pin_MISO);
    SPI.setPinMosi(Pin_MOSI);
    SPI.setPinSclk(Pin_Sclk);
    SPI.setClock(8000000);
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
}

EPD_Display::~EPD_Display() {}

EPD_Display& EPD_Display::display() {

    SPI.begin();
    _epd.begin();

    draw(last(), EPD_compensate);
    draw(last(), EPD_white);
    draw(active(), EPD_inverse);
    draw(active(), EPD_normal);

    _updates++;

    _dl = &last();

    _epd.end();
    SPI.end();

    return *this;

}

static void* addr;

void EPD_Display::draw(DisplayList& dl, EPD_stage stage) {
    RenderContext ib(dl.getPool(), dl.width(), 1, 0);
    addr = &ib;
    _epd.frame_fn_repeat([&](void *buffer, uint16_t line, uint16_t length) {
        // RenderContext& ib = *(RenderContext*)addr;
        // this method gets called back every time a line on the display is to be rendered.

        if (&ib != addr) {
            NRF51_CRASH("Addresses don't match.");
        }

        for(int i = 0; i <  ib.width/8; ++i) {
            *(((uint8_t*)buffer)+i) = 0;

            if (&ib != addr) {
                NRF51_CRASH("Addresses don't match.");
            }

        }

        if (&ib != addr) {
            NRF51_CRASH("Addresses don't match.");
        }

        ib.advance(0, line, (uint8_t*)buffer);
        dl.draw(ib);
        ib.finish();
        if (&ib != addr) {
            NRF51_CRASH("Addresses don't match.");
        }
    }, stage);
}