/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Aug 29, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <drivers/cs_PWM.h>
#include <protocol/cs_ErrorCodes.h>
#include <util/cs_Error.h>

PWM::PWM() : _initialized(false), _started(false), _startOnZeroCrossing(false) {}

uint32_t PWM::init(const pwm_config_t& config) {
    _initialized = true;
    return ERR_SUCCESS;
}

uint32_t PWM::deinit() {
    _initialized = false;
    return ERR_SUCCESS;
}

uint32_t PWM::start(bool onZeroCrossing) {
    _started = true;
    return ERR_SUCCESS;
}

void PWM::start() {
    _started = true;
}

void PWM::setValue(uint8_t channel, uint8_t value, uint8_t speed) {
    assert(_initialized, "not initialized");
    assert(channel < CS_PWM_MAX_CHANNELS, "channel out of bounds");
}

uint8_t PWM::getValue(uint8_t channel) {
    assert(_initialized, "not initialized");
    assert(channel < _config.channelCount, "channel out of bounds");

    return 0xff;
}
void PWM::onPeriodEnd() {}

void PWM::onZeroCrossingInterrupt() {
    assert(_initialized, "not initialized");

    if (!_started) {
        if (_startOnZeroCrossing) {
            start();
        }
        return;
    }
}

void PWM::_handleInterrupt() {}
