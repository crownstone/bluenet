/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: Jan. 30, 2015
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include <protocol/MeshControl.h>

#include <drivers/serial.h>
#include <drivers/nrf_pwm.h>

MeshControl::MeshControl() {
}

/**
 * Get incoming messages and perform certain actions.
 */
void MeshControl::process(uint8_t handle, uint32_t message) {
	LOGi("Process incoming mesh message");
	if (handle == 1) {
		if (message == 1) {
			LOGi("Turn lamp/device on");
			PWM::getInstance().setValue(0, (uint8_t)-1);
		} else if (message == 0) {
			LOGi("Turn lamp/device off");
			PWM::getInstance().setValue(0, 0);
		} else {
			LOGi("Don't know what to do with message with value %i", message);
		}
	}
}
