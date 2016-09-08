/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 19, 2016
 * License: LGPLv3+
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <drivers/cs_Timer.h>

#define EXTENDED_SWITCH_STATE

#ifdef EXTENDED_SWITCH_STATE
struct switch_state_t {
	uint8_t pwm_state : 7;
	uint8_t relay_state : 1;
};
#else
typedef uint8_t switch_state_t;
#endif

enum {
	SWITCH_NEXT_RELAY_VAL_NONE,
	SWITCH_NEXT_RELAY_VAL_ON,
	SWITCH_NEXT_RELAY_VAL_OFF,
};

class Switch {
public:
	//! Gets a static singleton (no dynamic memory allocation)
	static Switch& getInstance() {
		static Switch instance;
		return instance;
	}

	void init();

	void start();

	void pwmOff();

	void pwmOn();

	void turnOn();

	void turnOff();

	void dim(uint8_t value);

	void setPwm(uint8_t value);

	switch_state_t getValue();
	void setValue(switch_state_t value);

	uint8_t getPwm();
	bool getRelayState();

	void relayOn();

	void relayOff();

	static void staticTimedSetRelay(Switch* ptr) { ptr->timedSetRelay(); }

private:
	Switch();

	void updateSwitchState();

	void timedSetRelay();

	switch_state_t _switchValue;

#if (NORDIC_SDK_VERSION >= 11)
	app_timer_t              _relayTimerData;
	app_timer_id_t           _relayTimerId;
#else
	uint32_t                 _relayTimerId;
#endif

	uint8_t _nextRelayVal;
};

