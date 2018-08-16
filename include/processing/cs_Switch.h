/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 19, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <drivers/cs_Timer.h>
#include <events/cs_EventListener.h>
#include <cfg/cs_Boards.h>

#if BUILD_MESHING == 1
#include <protocol/cs_MeshMessageTypes.h>
#endif

#define SWITCH_ON 100

struct __attribute__((__packed__)) __attribute__((__aligned__(4))) switch_state_t {
	uint8_t pwm_state : 7;
	uint8_t relay_state : 1;
};

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
 *   - CONFIG_PWM_PERIOD          		Time between consecutive switch ON actions during PWM dim cycle
 * 	 - CONFIG_RELAY_HIGH_DURATION		Duration of time in which power is supplied to the relay in order to switch it
 * 
 * There are several things that influence the switch behavior (in order of importance):
 *   - IGBT overload (soft fuse).
 *   - Relay overload (soft fuse).
 *   - PWM powered (hardware restriction).
 *   - Dimming allowed (config).
 *   - Switch lock (config).
 *
 */
class Switch : EventListener {
public:
	//! Gets a static singleton (no dynamic memory allocation)
	static Switch& getInstance() {
		static Switch instance;
		return instance;
	}

	void init(const boards_config_t& board);

	void start();

	/** Starts the pwm, call when pwm is physically allowed to be used.
	 */
	void startPwm();

	/** Turn on the switch.
	 *
	 * See setSwitch()
	 */
	void turnOn();

	/** Turn off the switch. Uses relay if available.
	 *
	 * See setSwitch()
	 */
	void turnOff();

	/** Toggle the switch.
	 *
	 * See setSwitch()
	 */
	void toggle();


	/** Set switch state. Whether to use pwm or relay is automatically determined.
	 *
	 * Checks for switch lock.
	 * When dimming: turns on relay if pwm isn't powered up yet, then dimms when pwm is powered up.
	 * Stores state.
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
	 *
	 * See setPwm()
	 */
	void pwmOn();

	/** Turn off the PWM.
	 *
	 * See setPwm()
	 */
	void pwmOff();

	/** Set pwm value
	 *
	 * Checks for switch lock.
	 * Turns on relay if pwm isn't powered up yet, then dimms when pwm is powered up.
	 * Stores state.
	 *
	 *  @param[in] value                 Value to set the PWM to, ranges from 0-100.
	 */
	void setPwm(uint8_t value);

	/** Get the current PWM value.
	 *
	 * @return                           Current PWM value.
	 */
	uint8_t getPwm();


	/** Turn on the relay.
	 *
	 * Checks for switch lock.
	 * Stores state.
	 */
	void relayOn();

	/** Turn off the relay.
	 *
	 * Checks for switch lock.
	 * Stores state.
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

//	//! To be called on zero crossings.
//	void onZeroCrossing();

	/** Used internally
	 */
	static void staticTimedSwitch(Switch* ptr) { ptr->delayedSwitchExecute(); }

	/** Used internally
	 */
	static void staticTimedStoreSwitch(Switch* ptr) { ptr->delayedStoreStateExecute(); }

	/** Used internally
	 */
	void handleEvent(event_t & event);

#if BUILD_MESHING == 1
	/** Used internally
	 */
	void handleMultiSwitch(multi_switch_cmd_t* cmd);
#endif

private:
	Switch();

	/** Sets the pwm and udpates switch state.
	 *
	 * Does not store the state.
	 * Checks if dimming is allowed (by config and soft fuse).
	 * Does not check for switch lock.
	 *
	 * @param[in] value                  Pwm value (0-100)
	 * @return                           True when the given value was set, false otherwise.
	 */
	bool _setPwm(uint8_t value);

	/** Turns the relay on.
	 *
	 * Does not store the state.
	 * Checks if relay is allowed to be turned on (by soft fuse).
	 * Does not check for switch lock.
	 * Does not check if relay is already on.
	 */
	void _relayOn();

	/** Turns the relay off.
	 *
	 * Does not store the state.
	 * Checks if relay is allowed to be turned off (by soft fuse).
	 * Does not check for switch lock.
	 * Does not check if relay is already off.
	 */
	void _relayOff();

	//! Store the switch state.
	void storeState(switch_state_t oldVal);

	//! Store the switch state after a delay.
	void delayedStoreState(uint32_t delayMs);

	//! Store the switch state after the delay time has passed.
	void delayedStoreStateExecute();

	//! Set switch after the delay time has passed.
	void delayedSwitchExecute();

	void pwmNotAllowed();

	void forcePwmOff();
	void forceRelayOn();
	void forceSwitchOff();

	bool allowPwmOn();
	bool allowRelayOff();
	bool allowRelayOn();

	switch_state_t _switchValue;

	//! Timer used to set the switch state with a delay.
	app_timer_t              _switchTimerData;
	app_timer_id_t           _switchTimerId;

	//! Timer used to write the switch state to storage with a delay.
	app_timer_t              _switchStoreStateTimerData;
	app_timer_id_t           _switchStoreStateTimerId;

//	uint8_t _nextRelayVal;

	bool _pwmPowered; //! Whether or not the pwm has enough power to be used.
	bool _relayPowered; //! Whether or not the relay has enough power to be used.

	bool _hasRelay;
	uint8_t _pinRelayOn;
	uint8_t _pinRelayOff;
	uint16_t _relayHighDuration;

	bool _delayedSwitchPending;
	uint8_t _delayedSwitchState;

	bool _delayedStoreStatePending;

	uint32_t _hardwareBoard;

};
