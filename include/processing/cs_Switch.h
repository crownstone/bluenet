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
#include <cfg/cs_Boards.h>

#if BUILD_MESHING == 1
#include <protocol/cs_MeshMessageTypes.h>
#endif

struct __attribute__((__packed__)) switch_state_t {
	uint8_t pwm_state : 7;
	bool relay_state : 1;
};

//enum {
//	SWITCH_NEXT_RELAY_VAL_NONE,
//	SWITCH_NEXT_RELAY_VAL_ON,
//	SWITCH_NEXT_RELAY_VAL_OFF,
//};

class Switch : EventListener {
public:
	//! Gets a static singleton (no dynamic memory allocation)
	static Switch& getInstance() {
		static Switch instance;
		return instance;
	}

	void init(boards_config_t* board);

	void start();

	/** Turn on the switch. Uses relay if available.
	 */
	void turnOn();

	/** Turn off the switch. Uses relay if available.
	 */
	void turnOff();

	/** Toggle the switch. Uses relay if available.
	 */
	void toggle();


	/** Set switch state
	 *
	 * @param[in] switchState            State to set the switch to, includes both PWM and relay.
	 */
	void setSwitch(switch_state_t* switchState);

	/** Get the current switch state.
	 *
	 * @return                           Current switch state.
	 */
	switch_state_t getSwitchState();

	/** Turn on the PWM.
	 */
	void pwmOn();

	/** Turn off the PWM.
	 */
	void pwmOff();

	/** Set pwm value
	 *
	 *  @param[in] value                 Value to set the PWM to.
	 */
	void setPwm(uint8_t value);

	/** Get the current PWM value.
	 *
	 * @return                           Current PWM value.
	 */
	uint8_t getPwm();


	/** Turn on the relay.
	 */
	void relayOn();

	/** Turn off the relay.
	 */
	void relayOff();

	/** Get the current relay state.
	 *
	 * @return                           Whether or not the relay is on.
	 */
	bool getRelayState();


	/** Used internally
	 */
	static void staticTimedSwitch(Switch* ptr) { ptr->timedSwitch(); }

	/** Used internally
	 */
	void handleEvent(uint16_t evt, void* p_data, uint16_t length);

#if BUILD_MESHING == 1
	/** Used internally
	 */
	void handleMultiSwitch(multi_switch_item_t* p_item);
#endif

private:
	Switch();

	void _setPwm(uint8_t value);
	void _relayOn();
	void _relayOff();

	void updateSwitchState();

	void timedSwitch();

	void forcePwmOff();
	void forceSwitchOff();

	bool allowPwmOn();
	bool allowSwitchOn();

	void handleDelayed(switch_state_t* switchState, uint16_t delay);

	switch_state_t _switchValue;

	app_timer_t              _switchTimerData;
	app_timer_id_t           _switchTimerId;

//	uint8_t _nextRelayVal;

	bool _hasRelay;
	uint8_t _pinRelayOn;
	uint8_t _pinRelayOff;

	switch_state_t* _delayedSwitchState;
	uint16_t _relayHighDuration;

};

