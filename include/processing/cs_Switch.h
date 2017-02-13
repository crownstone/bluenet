/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: May 19, 2016
 * License: LGPLv3+
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <drivers/cs_Timer.h>
#include <events/cs_EventListener.h>

#if BUILD_MESHING == 1
#include <protocol/cs_MeshMessageTypes.h>
#endif

extern "C" {
#include <cfg/cs_Boards.h>
}

#define EXTENDED_SWITCH_STATE

#ifdef EXTENDED_SWITCH_STATE
struct switch_state_t {
	uint8_t pwm_state : 7;
	bool relay_state : 1;
};
#else
typedef uint8_t switch_state_t;
#endif

enum {
	SWITCH_NEXT_RELAY_VAL_NONE,
	SWITCH_NEXT_RELAY_VAL_ON,
	SWITCH_NEXT_RELAY_VAL_OFF,
};

class Switch : EventListener {
public:
	//! Gets a static singleton (no dynamic memory allocation)
	static Switch& getInstance() {
		static Switch instance;
		return instance;
	}

	void init(boards_config_t* board);

	void start();

	void pwmOff();

	void pwmOn();

	void turnOn();

	void turnOff();

	void toggle();

	void dim(uint8_t value);

	void setPwm(uint8_t value);

	switch_state_t getSwitchState();
//	void setValue(switch_state_t value);

	uint8_t getPwm();
	bool getRelayState();

	void relayOn();

	void relayOff();

	static void staticTimedSwitch(Switch* ptr) { ptr->timedSwitch(); }

	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

#if BUILD_MESHING == 1
	void handleMultiSwitch(multi_switch_item_t* p_item);
#endif

	void handle(switch_state_t* switchState);

	void handleDelayed(switch_state_t* switchState, uint16_t delay);

private:
	Switch();

	void updateSwitchState();

	void timedSwitch();

	void forcePwmOff();
	void forceSwitchOff();

	bool allowPwmOn();
	bool allowSwitchOn();

	switch_state_t _switchValue;

#if (NORDIC_SDK_VERSION >= 11)
	app_timer_t              _switchTimerData;
	app_timer_id_t           _switchTimerId;
#else
	uint32_t                 _switchTimerId;
#endif

//	uint8_t _nextRelayVal;

	bool _hasRelay;
	uint8_t _pinRelayOn;
	uint8_t _pinRelayOff;

	switch_state_t* _delayedSwitchState;

};

