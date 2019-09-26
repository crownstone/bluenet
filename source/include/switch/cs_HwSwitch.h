/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 26, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


/**
 * Provides the most low level interaction with the switch hardware.
 */
class HwSwitch{
    
	/** 
     * Turn on the PWM.
	 *
	 * See setPwm()
	 */
	void pwmOn();

	/** 
     * Turn off the PWM.
	 *
	 * See setPwm()
	 */
	void pwmOff();

    /** 
     * 
	 */
	void relayToggle();
};