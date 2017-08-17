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
	uint8_t relay_state : 1;
};

#define PWM_PIN 18

#define SWITCH_ON 100
//enum {
//	SWITCH_NEXT_RELAY_VAL_NONE,
//	SWITCH_NEXT_RELAY_VAL_ON,
//	SWITCH_NEXT_RELAY_VAL_OFF,
//};


/* Power Switch
 * 
 * The Switch class governs the two (separate) pathways that enable power to the load (switch ON) 
 * or disable power to the load (switch OFF)
 * The two pathways are:
 * * the Relay (for high current loads but slow switching times)
 * * the IGBTs (for low current loads but fast switching times)
 * Switching the IGBT pathway is done through Pulse Width Modulation (PWM)
 * 
 * #define variables used in this class
 * 	 - PWM_DISABLE						If this is defined, the Switch class will ignore all attempts to switch PWM
 *   - CONFIG_PWM_PERIOD          		Time between consecutive switch ON actions during PWM dim cycle
 * 	 - CONFIG_RELAY_HIGH_DURATION		Duration of time in which power is supplied to the relay in order to switch it
 * 
 * */
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


	/** Set switch state. Whether to use pwm or relay is automatically determined.
	 *
	 * @param[in] switchState            State to set the switch to, ranges from 0-100.
	 */
	void setSwitch(uint8_t switchState);

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

	/** Set switch state with a delay.
	 *  Will be canceled if another switch or delayed switch is called.
	 *
	 * @param[in] switchState            State to set the switch to, ranges from 0-100.
	 * @param[in] delay                  Delay in seconds.
	 */
	void delayedSwitch(uint8_t switchState, uint16_t delay);

	void onZeroCrossing();


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

	void updateSwitchState(switch_state_t oldVal);

	void timedSwitch();

	void forcePwmOff();
	void forceSwitchOff();

	bool allowPwmOn();
	bool allowSwitchOn();

	switch_state_t _switchValue;

	app_timer_t              _switchTimerData;
	app_timer_id_t           _switchTimerId;

//	uint8_t _nextRelayVal;

	bool _hasRelay;
	uint8_t _pinRelayOn;
	uint8_t _pinRelayOff;
	uint16_t _relayHighDuration;

	bool _delayedSwitchPending;
	uint8_t _delayedSwitchState;

};

