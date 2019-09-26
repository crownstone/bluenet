/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 26, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <cfg/cs_Boards.h>


/**
 * Provides the most low level interaction with the switch hardware.
 */
class HwSwitch{
    
    // Functions copied from cs_Switch:
    void relayOn();
    void relayOff();
    bool setPwm(uint8_t value);
    void setSwitch(uint8_t switchState);
    void startPwm();
    void init(const boards_config_t& board);
    // end of functions copied from cs_Switch


	void relayToggle();
};