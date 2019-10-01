/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: May 19, 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <processing/cs_Switch.h>

#include <cfg/cs_Boards.h>
#include <cfg/cs_Config.h>
#include <drivers/cs_Serial.h>

#include <storage/cs_State.h>

#include <cfg/cs_Strings.h>
#include <ble/cs_Stack.h>
#include <processing/cs_Scanner.h>
#include <events/cs_EventDispatcher.h>
#include <drivers/cs_PWM.h>

#define PRINT_SWITCH_VERBOSE

/**
 * The Switch class takes care of switching loads and configures also the Dimmer.
 */
Switch::Switch() : swSwitch() {
	LOGd(FMT_CREATE, "switch");
	_switchTimerData = { {0} };
	_switchTimerId = &_switchTimerData;
}

/**
 * TODO: Remove function. This should be self-contained... If it is required to wait a bit after init, this type of
 * decision should be part of this class. If there is a message exchange required with the current sensing module,
 * then this should be done over the event bus.
 *
 * The PWM is required for syncing with zero-crossings.
 */
void Switch::start() {
	LOGd("Start switch");

	_relayPowered = true;

	// Already start PWM, so it can sync with the zero crossings. But don't set the value yet.
	PWM::getInstance().start(true);

	// If switchcraft is enabled, assume a boot is due to a brownout caused by a too slow wall switch.
	// This means we will assume that the pwm is already powered and just set the _pwmPowered flag.
	// TODO: Really? Why can't we just organize this with events?
	bool switchcraftEnabled = State::getInstance().isTrue(CS_TYPE::CONFIG_SWITCHCRAFT_ENABLED);
	if (switchcraftEnabled || (PWM_BOOT_DELAY_MS == 0) || _hardwareBoard == ACR01B10B || _hardwareBoard == ACR01B10C) {
		LOGd("dimmer powered on start");
		_pwmPowered = true;
		swSwitch->setDimmer(true);
		
		event_t event(CS_TYPE::EVT_DIMMER_POWERED, &_pwmPowered, sizeof(_pwmPowered));
		EventDispatcher::getInstance().dispatch(event);
	}

	if (_switchValue.state.dimmer != 0) {
		if (_pwmPowered) {
			swSwitch->setIntensity(_switchValue.state.dimmer);
		}
		else {
			// This is in case of a wall switch.
			// You don't want to hear the relay turn on and off every time you power the crownstone.
			// TODO: So, why does it call _relayOn() if it is supposed to do nothing...?
			swSwitch->setRelay(true);
		}
	}
	else {
		// Make sure the relay is in the stored position (no need to store).
		// TODO: Why is it not stored?
		if (_switchValue.state.relay == 1) {
			swSwitch->setRelay(true);
		}
		else {
			swSwitch->setRelay(false);
		}
	}
}

// /**
//  * Store the state immediately when relay value changed, as the dim value often changes a lot in bursts.
//  * Compare it against given oldVal, instead of switch state stored in State, as that is always changed immediately
//  * (just not the value on flash).
//  */
// void Switch::storeState(switch_state_t oldVal) {
// 	bool persistNow = false;
// 	if (oldVal.asInt != _switchValue.asInt) {
// 		LOGd("Store switch state %i, %i", _switchValue.state.relay, _switchValue.state.dimmer);
// 		persistNow = (oldVal.state.relay != _switchValue.state.relay);
// 	}

// 	cs_state_data_t stateData(CS_TYPE::STATE_SWITCH_STATE, (uint8_t*)&_switchValue, sizeof(_switchValue));
// 	if (persistNow) {
// 		State::getInstance().set(stateData);
// 	}
// 	else {
// 		State::getInstance().setDelayed(stateData, SWITCH_DELAYED_STORE_MS / 1000);
// 	}
// }

// switch_state_t Switch::getSwitchState() {
// #ifdef PRINT_SWITCH_VERBOSE
// //LOGd(FMT_GET_INT_VAL, "Switch state", _switchValue);
// 	LOGd("Switch state %i, %i", _switchValue.state.relay, _switchValue.state.dimmer);
// #endif
// 	return _switchValue;
// }


// void Switch::turnOn() {
// #ifdef PRINT_SWITCH_VERBOSE
// 	LOGd("Turn ON");
// #endif
// 	setSwitch(99);
// }


// void Switch::turnOff() {
// #ifdef PRINT_SWITCH_VERBOSE
// 	LOGd("Turn OFF");
// #endif
// 	setSwitch(0);
// }

// void Switch::toggle() {
// 	// TODO: maybe check if pwm is larger than some value?
// 	if (_switchValue.state.relay == 1 || _switchValue.state.dimmer > 0) {
// 		setSwitch(0);
// 	}
// 	else {
// 		setSwitch(99);
// 	}
// }

// void Switch::pwmOff() {
// 	setPwm(0);
// }

// void Switch::pwmOn() {
// 	setPwm(SWITCH_ON);
// }

// void Switch::setPwm(uint8_t intensity){
// 	swSwitch->setIntensity(intensity);
// }

// void Switch::relayOn() {
// 	if (State::getInstance().isTrue(CS_TYPE::CONFIG_SWITCH_LOCKED)) {
// 		LOGw("Switch locked!");
// 		return;
// 	}
// 	switch_state_t oldVal = _switchValue;
	
// 	_relayOn();
	
// 	storeState(oldVal);
// }

/**
 * Wrapper function that has all kind of side-effects besides turning off the relay. It does the following things:
 *   - check if the relay is locked, it will not switch in that case
 *   - turn off the relay
 *   - store the old state of the relay
 * TODO: Use a struct { bool check_log; switch_state_t value; bool store_previous; value_t delay; }
 * TODO: Get rid of almost similar functions relayOff(), _relayOff(), forceRelayOff().
 * TODO: Get rid of almost similar functions with relay/switch/pwm.
 */
// void Switch::relayOff() {
// 	if (State::getInstance().isTrue(CS_TYPE::CONFIG_SWITCH_LOCKED)) {
// 		LOGw("Switch locked!");
// 		return;
// 	}
// 	switch_state_t oldVal = _switchValue;

// 	_relayOff();

// 	storeState(oldVal);
// }




// void Switch::delayedSwitch(uint8_t switchState, uint16_t delay) {

// #ifdef PRINT_SWITCH_VERBOSE
// 	LOGi("trigger delayed switch state: %d, delay: %d", switchState, delay);
// #endif

// 	if (delay == 0) {
// 		LOGw("delay can't be 0");
// 		delay = 1;
// 	}

// 	if (_delayedSwitchPending) {
// #ifdef PRINT_SWITCH_VERBOSE
// 		LOGi("clear existing delayed switch state");
// #endif
// 		Timer::getInstance().stop(_switchTimerId);
// 	}

// 	_delayedSwitchPending = true;
// 	_delayedSwitchState = switchState;
// 	Timer::getInstance().start(_switchTimerId, MS_TO_TICKS(delay * 1000), this);
// }


// void Switch::delayedSwitchExecute() {
// #ifdef PRINT_SWITCH_VERBOSE
// 	LOGi("execute delayed switch");
// #endif

// 	if (_delayedSwitchPending) {
// 		setSwitch(_delayedSwitchState);
// 	}
// }

// void Switch::pwmNotAllowed() {
// 	switch_state_t oldVal = _switchValue;
// 	if (_switchValue.state.dimmer != 0) {
// 		_setPwm(0);
// 		_relayOn();
// 	}
// 	storeState(oldVal);
// }

// void Switch::forcePwmOff() {
// 	LOGw("forcePwmOff");
// 	switch_state_t oldVal = _switchValue;
// 	_setPwm(0);
// 	storeState(oldVal);
// 	event_t event(CS_TYPE::EVT_DIMMER_FORCED_OFF);
// 	EventDispatcher::getInstance().dispatch(event);
// }

// void Switch::forceRelayOn() {
// 	LOGw("forceRelayOn");
// 	switch_state_t oldVal = _switchValue;
// 	_relayOn();
// 	storeState(oldVal);
// 	event_t event(CS_TYPE::EVT_RELAY_FORCED_ON);
// 	EventDispatcher::getInstance().dispatch(event);
// 	// Try again later, in case the first one didn't work..
// 	delayedSwitch(SWITCH_ON, 5);
// }

// void Switch::forceSwitchOff() {
// 	LOGw("forceSwitchOff");
// 	switch_state_t oldVal = _switchValue;
// 	_setPwm(0);
// 	_relayOff();
// 	storeState(oldVal);
// 	event_t event(CS_TYPE::EVT_SWITCH_FORCED_OFF);
// 	EventDispatcher::getInstance().dispatch(event);
// 	// Try again later, in case the first one didn't work..
// 	delayedSwitch(0, 5);
// }

// bool Switch::allowPwmOn() {
// 	TYPIFY(STATE_ERRORS) stateErrors;
// 	State::getInstance().get(CS_TYPE::STATE_ERRORS, &stateErrors, sizeof(stateErrors));
// 	LOGd("errors=%d", stateErrors.asInt);

// 	return !(
// 		stateErrors.errors.chipTemp 
// 		|| stateErrors.errors.overCurrent 
// 		|| stateErrors.errors.overCurrentDimmer 
// 		|| stateErrors.errors.dimmerTemp 
// 		|| stateErrors.errors.dimmerOn);
// }

// bool Switch::allowRelayOff() {
// 	TYPIFY(STATE_ERRORS) stateErrors;
// 	State::getInstance().get(CS_TYPE::STATE_ERRORS, &stateErrors, sizeof(stateErrors));

// 	// When dimmer has (had) problems, protect the dimmer by keeping the relay on.
// 	return !(
// 		stateErrors.errors.overCurrentDimmer 
// 		|| stateErrors.errors.dimmerTemp 
// 		|| stateErrors.errors.dimmerOn);
// }

// bool Switch::allowRelayOn() {
// 	TYPIFY(STATE_ERRORS) stateErrors;
// 	State::getInstance().get(CS_TYPE::STATE_ERRORS, &stateErrors, sizeof(stateErrors));
// 	LOGd("errors=%d", stateErrors.asInt);

// 	return 
// 		// When dimmer has (had) problems, protect the dimmer by keeping the relay on.
// 		!allowRelayOff()  
// 		// Otherwise, relay should stay off when the overall temperature was too high, or there was over current.
// 		|| !(stateErrors.errors.chipTemp || stateErrors.errors.overCurrent);
// }

// void Switch::checkDimmerPower() {
// 	// Check if dimmer is on, but power usage is low.
// 	// In that case, assume the dimmer isn't working due to a cold boot.
// 	// So turn dimmer off and relay on.
// 	if (_switchValue.state.dimmer == 0) {
// 		return;
// 	}

// 	TYPIFY(STATE_POWER_USAGE) powerUsage;
// 	State::getInstance().get(CS_TYPE::STATE_POWER_USAGE, &powerUsage, sizeof(powerUsage));
// 	TYPIFY(CONFIG_POWER_ZERO) powerZero;
// 	State::getInstance().get(CS_TYPE::CONFIG_POWER_ZERO, &powerZero, sizeof(powerZero));

// 	LOGd("powerUsage=%i powerZero=%i", powerUsage, powerZero);

// 	if (powerUsage < DIMMER_BOOT_CHECK_POWER_MW 
// 		|| (powerUsage < DIMMER_BOOT_CHECK_POWER_MW_UNCALIBRATED 
// 			&& powerZero == CONFIG_POWER_ZERO_INVALID)
// 		) {
// 		_pwmPowered = false;

// 		event_t event(CS_TYPE::EVT_DIMMER_POWERED, &_pwmPowered, sizeof(_pwmPowered));
// 		EventDispatcher::getInstance().dispatch(event);

// 		setSwitch(SWITCH_ON);
// 		return;
// 	}
// }

/**
 * For now, only deal with device tokens and let all other sources overrule (but not actually claim).
 */
bool Switch::checkAndSetOwner(cmd_source_t source) {
	if (source.sourceId < CS_CMD_SOURCE_DEVICE_TOKEN) {
		// Non device token command can always set the switch.
		return true;
	}
	if (_ownerTimeoutCountdown == 0) {
		// Switch isn't claimed yet.
		_source = source;
		_ownerTimeoutCountdown = SWITCH_CLAIM_TIME_MS / TICK_INTERVAL_MS;
		return true;
	}
	if (_source.sourceId != source.sourceId) {
		// Switch is claimed by other source.
		LOGd("Already claimed by %u", _source.sourceId);
		return false;
	}
	if (!BLEutil::isNewer(_source.count, source.count)) {
		// A command with newer counter has been received already.
		LOGd("Old command: %u, already got: %u", source.count, _source.count);
		return false;
	}
	_source = source;
	_ownerTimeoutCountdown = SWITCH_CLAIM_TIME_MS / TICK_INTERVAL_MS;
	return true;
}

void Switch::handleEvent(event_t & event) {
	switch(event.type) {
		case CS_TYPE::CMD_SWITCH_ON:
			if(swSwitch) swSwitch->setIntensity(100);
			break;
		case CS_TYPE::CMD_SWITCH_OFF:
			if(swSwitch) swSwitch->setIntensity(0);
			break;
		case CS_TYPE::CMD_SWITCH_TOGGLE:
			if(swSwitch) swSwitch->toggle();
			break;
		case CS_TYPE::CMD_SWITCH: {
			TYPIFY(CMD_SWITCH)* packet = (TYPIFY(CMD_SWITCH)*) event.data;
			// if (packet->delay) {
			// 	delayedSwitch(packet->switchCmd, packet->delay);
			// }
			// else {
				if (checkAndSetOwner(packet->source)) {
					if(swSwitch) swSwitch->setIntensity(packet->switchCmd);
				}
			// }
			break;
		}
// 		case CS_TYPE::EVT_TICK:
// 			if (_dimmerCheckCountdown) {
// 				if (--_dimmerCheckCountdown == 0) {
// 					checkDimmerPower();
// 				}
// 			}
// 			if (_ownerTimeoutCountdown) {
// 				--_ownerTimeoutCountdown;
// //				if (--_ownerTimeoutCountdown == 0) {
// //					_source.flagExternal = false;
// //					_source.sourceId = CS_CMD_SOURCE_NONE;
// //					_source.count = 0;
// //				}
// 			}
// 			break;
		default: {}
	}
}


// ================= HwSwitch Wrapper =================

// void Switch::_relayOn(){
// 	if (!allowRelayOn()) {
// 		return;
// 	}

// 	hwSwitch.relayOn();

// 	_switchValue.state.relay = 1;
// }

// void Switch::_relayOff() {
// 	if (!allowRelayOff()) {
// 		return;
// 	}

// 	hwSwitch.relayOff();

// 	_switchValue.state.relay = 0;
// }

// bool Switch::_setPwm(uint8_t value){
// 	if (value > 0 && !allowPwmOn()) {
// 		return false;
// 	}

// 	bool pwm_allowed = State::getInstance().isTrue(CS_TYPE::CONFIG_PWM_ALLOWED);
// 	if (value != 0 && !pwm_allowed) {
// 		_switchValue.state.dimmer = 0;
// 		return false;
// 	}

// 	// When the user wants to dim at 99%, assume the user actually wants full on, but doesn't want to use the relay.
// 	if (value >= (SWITCH_ON - 1)) {
// 		value = SWITCH_ON;
// 	}
// 	_switchValue.state.dimmer = value;
// 	if (value != 0 && !_pwmPowered) {
// 		// State stored, but not executed yet, so return false.
// 		return false;
// 	}

// 	swSwitch->setIntensity(value);

// 	return true;
// }

void Switch::startPwm(){
	if (_pwmPowered) {
		// early return
		return;
	}
	_pwmPowered = true;
	
	swSwitch->setDimmer(true);

	// sw switch arranges that needs to happen on hw level, and it
	// persists whatever data it sees necessary
	swSwitch->setIntensity(_switchValue.state.dimmer); 

	event_t event(CS_TYPE::EVT_DIMMER_POWERED, &_pwmPowered, sizeof(_pwmPowered));
	EventDispatcher::getInstance().dispatch(event);
}

// void Switch::setSwitch(uint8_t switchState) {
// 	switch_state_t oldVal = _switchValue;
// 	if (State::getInstance().isTrue(CS_TYPE::CONFIG_SWITCH_LOCKED)) {
// 		return;
// 	}

// 	switch (_hardwareBoard) {
// 		case ACR01B1A: {
// 			// Always use the relay
// 			if (switchState) {
// 				_relayOn();
// 			}
// 			else {
// 				_relayOff();
// 			}
// 			_setPwm(0);
// 			break;
// 		}
// 		default: {
// 			// TODO: the pwm gets set at the start of a period, which lets the light flicker in case the relay is turned off..
// 			// First pwm on, then relay off!
// 			// Otherwise, if you go from 100 to 90, the power first turns off, then to 90.
// 			// TODO: why not first relay on, then pwm off, when going from 90 to 100?

// 			// Pwm when value is 1-99, else pwm off
// 			bool pwmOnSuccess = true;
// 			if (switchState > 0 && switchState < SWITCH_ON) {
// 				pwmOnSuccess = _setPwm(switchState);
// 			}
// 			else {
// 				_setPwm(0);
// 			}

// 			// Relay on when value >= 100, or when trying to dim, but that was unsuccessful.
// 			// Else off (as the dimmer is parallel)
// 			if (switchState >= SWITCH_ON || !pwmOnSuccess) {
// 				if (_switchValue.state.relay == 0) {
// 					_relayOn();
// 				}
// 			}
// 			else {
// 				if (_switchValue.state.relay == 1) {
// 					_relayOff();
// 				}
// 			}
// 			break;
// 		}
// 	}

// 	// The new value overrules a timed switch.
// 	if (_delayedSwitchPending) {
// 		Timer::getInstance().stop(_switchTimerId);
// 		_delayedSwitchPending = false;
// 	}

// 	storeState(oldVal);
// }

void Switch::init(const boards_config_t& board){
	// Note: for SwitchAggregator these obtained values extracted as parameters.
	TYPIFY(CONFIG_PWM_PERIOD) pwmPeriod;
	State::getInstance().get(CS_TYPE::CONFIG_PWM_PERIOD, &pwmPeriod, sizeof(pwmPeriod));

	uint16_t relayHighDuration = 0;
	State::getInstance().get(CS_TYPE::CONFIG_RELAY_HIGH_DURATION, &relayHighDuration, sizeof(relayHighDuration));

	HwSwitch hwSwitch(board, pwmPeriod, relayHighDuration);
	swSwitch = SwSwitch(hwSwitch);

	// Retrieve last switch state from persistent storage

	State::getInstance().get(CS_TYPE::STATE_SWITCH_STATE, &_switchValue, sizeof(_switchValue));

	EventDispatcher::getInstance().addListener(this);
	// Timer::getInstance().createSingleShot(_switchTimerId,
	// 		(app_timer_timeout_handler_t)Switch::staticTimedSwitch);
}