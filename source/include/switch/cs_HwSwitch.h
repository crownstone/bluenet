/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 26, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <cfg/cs_Boards.h>

#include <common/cs_Types.h>

/**
 * Provides the most low level interaction with the switch hardware.
 */
class HwSwitch{
    public:
    
    // Functions copied from cs_Switch:
    void relayOn();
    void relayOff();
    void setPwm(uint8_t value);
    
    void startPwm();
    void init(const boards_config_t& board, uint32_t pwmPeriod, uint16_t relayHighDuration);
    
    
    bool _hasRelay;
    uint8_t _pinRelayOn;
    uint8_t _pinRelayOff;
    uint8_t _pinEnableDimmer = 0;
    TYPIFY(CONFIG_RELAY_HIGH_DURATION) _relayHighDuration = 0;
    // end of functions copied from cs_Switch

    void enableDimmer();
	void relayToggle();
};