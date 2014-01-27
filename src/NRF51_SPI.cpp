

#include "NRF51_SPI.h"

NRF51_SPIClass::NRF51_SPIClass(uint8_t unit) :
    _unit(unit),
    _pin_sclk(255),
    _pin_mosi(255),
    _pin_miso(255),
    _freq(NRF51_SPI_FREQUENCY_1M),
    _timeout(0x3000),
    _ok(false),
    _pulls(false) {}

void NRF51_SPIClass::begin() {
    if (_pin_sclk == 255 || _pin_miso == 255 || _pin_mosi == 255) {
        _ok = false;
        return;
    }

    NRF51_GPIO_PIN_CNF(_pin_sclk) = NRF51_GPIO_PIN_CNF_OUTPUT;
    NRF51_SPI_PSELSCK(_unit) = _pin_sclk;

    NRF51_GPIO_PIN_CNF(_pin_mosi) = NRF51_GPIO_PIN_CNF_OUTPUT;
    NRF51_SPI_PSELMOSI(_unit) = _pin_mosi;

    NRF51_GPIO_PIN_CNF(_pin_miso) &= NRF51_GPIO_PIN_CNF_INPUT;
    NRF51_SPI_PSELMOSI(_unit) = _pin_mosi;

    if (_pulls) {
        NRF51_GPIO_PIN_CNF(_pin_miso) |= NRF51_GPIO_PIN_CNF_PULL_PULLUP;
        NRF51_GPIO_PIN_CNF(_pin_mosi) |= NRF51_GPIO_PIN_CNF_PULL_PULLUP;
        NRF51_GPIO_PIN_CNF(_pin_sclk) |= NRF51_GPIO_PIN_CNF_PULL_PULLDOWN;
    }

    NRF51_SPI_FREQUENCY(_unit) = _freq;

    NRF51_SPI_READY(_unit) = 0;

    NRF51_SPI_ENABLE(_unit) = 1;

    _ok = true;

}

void NRF51_SPIClass::setBitOrder(uint8_t bo) {
    if (bo == LSBFIRST) {
        NRF51_SPI_CONFIG(_unit) |= NRF51_SPI_CONFIG_LSBFIRST;
    } else if (bo == MSBFIRST) {
        NRF51_SPI_CONFIG(_unit) &= NRF51_SPI_CONFIG_MSBFIRST;
    }
}

void NRF51_SPIClass::setDataMode(uint8_t dataMode) {
    switch(dataMode) {

        case SPI_MODE1:  // trailing, active high.
            NRF51_SPI_CONFIG(_unit) |= NRF51_SPI_CONFIG_CPHA_TRAILING;
            NRF51_SPI_CONFIG(_unit) &= NRF51_SPI_CONFIG_ACTIVE_HIGH;
            break;

        case SPI_MODE2:  // leading, active low.
            NRF51_SPI_CONFIG(_unit) &= NRF51_SPI_CONFIG_CPHA_LEADING;
            NRF51_SPI_CONFIG(_unit) |= NRF51_SPI_CONFIG_ACTIVE_LOW;
            break;

        case SPI_MODE3:  // trailing, active low.
            NRF51_SPI_CONFIG(_unit) |= NRF51_SPI_CONFIG_CPHA_TRAILING;
            NRF51_SPI_CONFIG(_unit) &= NRF51_SPI_CONFIG_ACTIVE_HIGH;
            break;

        case SPI_MODE0:  // leading, active high.
        default:
            NRF51_SPI_CONFIG(_unit) &= NRF51_SPI_CONFIG_CPHA_LEADING;
            NRF51_SPI_CONFIG(_unit) &= NRF51_SPI_CONFIG_ACTIVE_HIGH;
            break;
    }
}

void NRF51_SPIClass::setClock(uint32_t freq) {
    uint32_t best = NRF51_SPI_FREQUENCY_125_K;
    uint32_t error = UINT32_MAX;
    testFreq(NRF51_SPI_FREQUENCY_125_K, 125000, freq, &best, &error);
    testFreq(NRF51_SPI_FREQUENCY_250K, 250000, freq, &best, &error);
    testFreq(NRF51_SPI_FREQUENCY_1M, 1000000, freq, &best, &error);
    testFreq(NRF51_SPI_FREQUENCY_2M, 2000000, freq, &best, &error);
    testFreq(NRF51_SPI_FREQUENCY_4M, 4000000, freq, &best, &error);
    testFreq(NRF51_SPI_FREQUENCY_8M, 8000000, freq, &best, &error);

    _freq = best;
}

void NRF51_SPIClass::testFreq(uint32_t test, uint32_t val, uint32_t freq, uint32_t* best, uint32_t* error) {
    if (absdiff(val,freq) < *error) {
        *best = test;
        *error = absdiff(val,freq);
    }
}

uint32_t NRF51_SPIClass::absdiff(uint32_t a, uint32_t b) {
    return (a < b) ? b-a : a - b;
}

NRF51_SPIClass NRF51_SPI_0(0);
NRF51_SPIClass NRF51_SPI_1(1);
