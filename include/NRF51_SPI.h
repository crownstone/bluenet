

#ifndef _t3spi_h
#define _t3spi_h



#include "Arduino.h"

#include "nRF51822.h"


#define SPI_CLOCK_DIV2 0b0000
#define SPI_CLOCK_DIV4 0b0001
#define SPI_CLOCK_DIV6 0b0010
#define SPI_CLOCK_DIV8 0b0011
#define SPI_CLOCK_DIV16 0b0100
#define SPI_CLOCK_DIV32 0b0101
#define SPI_CLOCK_DIV64 0b0110
#define SPI_CLOCK_DIV128 0b0111

#define SPI_DELAY_DIV2 0b0000
#define SPI_DELAY_DIV4 0b0001
#define SPI_DELAY_DIV8 0b0010
#define SPI_DELAY_DIV16 0b0011
#define SPI_DELAY_DIV32 0b0100
#define SPI_DELAY_DIV64 0b0101
#define SPI_DELAY_DIV128 0b0110
#define SPI_DELAY_DIV256 0b0111
#define SPI_DELAY_DIV512 0b1000
#define SPI_DELAY_DIV1024 0b1000

#define SPI_MODE0 0x00
#define SPI_MODE1 0x01
#define SPI_MODE2 0x02
#define SPI_MODE3 0x03

class NRF51_SPIClass {
    uint8_t _unit;
    uint8_t _pin_sclk;
    uint8_t _pin_mosi;
    uint8_t _pin_miso;
    uint32_t _freq;
    uint32_t _timeout;
    uint32_t _delay;
    bool _ok;
    bool _pulls;


public:
    NRF51_SPIClass(uint8_t unit = 0);

    // SPI Configuration methods

    //inline void attachInterrupt();
    //inline void detachInterrupt(); // Default

    inline uint8_t transfer(uint8_t data) {
        _ok = true;
        NRF51_SPI_TXD(_unit) = data;
        uint32_t count = 0;
        while((!NRF51_SPI_READY(_unit)) && count < _timeout) {
            count++;
        }
        if (count == _timeout) {
            _ok = false;
            return 0;
        } else {
            _ok = true;
            NRF51_SPI_READY(_unit) = 0; // clear ready bit.
            return NRF51_SPI_RXD(_unit);
        }
    }

    operator bool() {
        return _ok;
    }

    /** Which SPI device to use (0 or 1).  Note that SPI and TWI share registers, so for a given unit, it
    * can only either be assigned to SPI or to TWI.  */
    void setSpiUnit(uint8_t unit) {
        _unit = unit;
    }

    void setTimeout(uint32_t timeout) {
        _timeout = timeout;
    }

    void setPinSclk(uint8_t pin) {
        _pin_sclk = pin;
    }

    void setPinMiso(uint8_t pin) {
        _pin_miso = pin;
    }

    void setPinMosi(uint8_t pin) {
        _pin_mosi = pin;
    }

    void setEnablePullResistors(bool pulls) {
        _pulls = pulls;
    }

    void begin();

    void end() {
        NRF51_SPI_ENABLE(_unit) = 0;
    }

    void setBitOrder(uint8_t bo);
    void setDataMode(uint8_t dataMode);

    void setClockDivider(uint8_t cdiv) {
        // According to http://arduino.cc/en/Reference/SPISetClockDivider
        // 16 MHz is base freq
        setClock(16000000 / cdiv);
    }

    void setClock(uint32_t freq);

    void setDelayAfterTransfer(uint32_t delayMicros) {
        // TODO
    }

  private:

    void testFreq(uint32_t test, uint32_t val, uint32_t freq, uint32_t* best, uint32_t* error);

    uint32_t absdiff(uint32_t a, uint32_t b);
};


extern NRF51_SPIClass NRF51_SPI_0;
extern NRF51_SPIClass NRF51_SPI_1;

#endif /* _t3spi_h */