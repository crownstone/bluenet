/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 26, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <cfg/cs_Boards.h>
#include <common/cs_Types.h>
#include <switch/cs_ISwitch.h>

/**
 * Provides the most low level interaction with the switch hardware.
 */
class HwSwitch : public ISwitch {
    public:
    
    virtual void setRelay(bool is_on) override;
    virtual void setDimmerPower(bool is_on) override;
    virtual void setIntensity(uint8_t value) override;
    
    /**
     * configures the PWM module and the gpio relay on and relay off pins.
     */
    HwSwitch(const boards_config_t& board, uint32_t pwmPeriod, uint16_t relayHighDuration);

    private:
    
    bool _hasRelay = false;
    uint8_t _pinRelayOn = 0;
    uint8_t _pinRelayOff = 0;
    uint8_t _pinEnableDimmer = 0;
    TYPIFY(CONFIG_RELAY_HIGH_DURATION) _relayHighDuration = 0;

    void relayOn();
    void relayOff();
    void enableDimmer();
    void disableDimmer();
};
