#include "switch/cs_HwSwitch.h"

void Switch::_relayOn() {
	LOGd("relayOn");
	if (!allowRelayOn()) {
		LOGi("Relay on not allowed");
		return;
	}


	if (_hasRelay) {
#ifdef PRINT_SWITCH_VERBOSE
		LOGd("trigger relay on pin for %d ms", _relayHighDuration);
#endif
		nrf_gpio_pin_set(_pinRelayOn);
		nrf_delay_ms(_relayHighDuration);
		nrf_gpio_pin_clear(_pinRelayOn);
	}
	_switchValue.state.relay = 1;
}

void Switch::_relayOff() {
	LOGd("relayOff");
	if (!allowRelayOff()) {
		LOGi("Relay off not allowed");
		return;
	}

	if (_hasRelay) {
#ifdef PRINT_SWITCH_VERBOSE
		LOGi("trigger relay off pin for %d ms", _relayHighDuration);
#endif
		nrf_gpio_pin_set(_pinRelayOff);
		nrf_delay_ms(_relayHighDuration);
		nrf_gpio_pin_clear(_pinRelayOff);
	}
	_switchValue.state.relay = 0;
}

bool Switch::_setPwm(uint8_t value) {
	LOGd("Set dimming to %u", value);
	if (value > 0 && !allowPwmOn()) {
		LOGi("Don't turn on pwm");
		return false;
	}

	bool pwm_allowed = State::getInstance().isTrue(CS_TYPE::CONFIG_PWM_ALLOWED);
	LOGd("Dimming allowed: %u", pwm_allowed);
	if (value != 0 && !pwm_allowed) {
		LOGd("Dimming not allowed");
		_switchValue.state.dimmer = 0;
		return false;
	}

	// When the user wants to dim at 99%, assume the user actually wants full on, but doesn't want to use the relay.
	if (value >= (SWITCH_ON - 1)) {
		value = SWITCH_ON;
	}
	_switchValue.state.dimmer = value;
	if (value != 0 && !_pwmPowered) {
		// State stored, but not executed yet, so return false.
		return false;
	}
	PWM::getInstance().setValue(0, value);
	return true;
}

void Switch::setSwitch(uint8_t switchState) {
#ifdef PRINT_SWITCH_VERBOSE
	LOGi("Set switch state: %d", switchState);
#endif
	switch_state_t oldVal = _switchValue;
	if (State::getInstance().isTrue(CS_TYPE::CONFIG_SWITCH_LOCKED)) {
		LOGw("Switch locked!");
		return;
	}

	switch (_hardwareBoard) {
		case ACR01B1A: {
			// Always use the relay
			if (switchState) {
				_relayOn();
			}
			else {
				_relayOff();
			}
			_setPwm(0);
			break;
		}
		default: {
			// TODO: the pwm gets set at the start of a period, which lets the light flicker in case the relay is turned off..
			// First pwm on, then relay off!
			// Otherwise, if you go from 100 to 90, the power first turns off, then to 90.
			// TODO: why not first relay on, then pwm off, when going from 90 to 100?

			// Pwm when value is 1-99, else pwm off
			bool pwmOnSuccess = true;
			if (switchState > 0 && switchState < SWITCH_ON) {
				pwmOnSuccess = _setPwm(switchState);
			}
			else {
				_setPwm(0);
			}

			// Relay on when value >= 100, or when trying to dim, but that was unsuccessful.
			// Else off (as the dimmer is parallel)
			if (switchState >= SWITCH_ON || !pwmOnSuccess) {
				if (_switchValue.state.relay == 0) {
					_relayOn();
				}
			}
			else {
				if (_switchValue.state.relay == 1) {
					_relayOff();
				}
			}
			break;
		}
	}

	// The new value overrules a timed switch.
	if (_delayedSwitchPending) {
#ifdef PRINT_SWITCH_VERBOSE
		LOGi("clear delayed switch state");
#endif
		Timer::getInstance().stop(_switchTimerId);
		_delayedSwitchPending = false;
	}

	storeState(oldVal);
}

void Switch::startPwm() {
	if (_pwmPowered) {
		return;
	}
	LOGd("dimmer powered");
	_pwmPowered = true;
	nrf_gpio_pin_set(_pinEnableDimmer);

	// Restore the pwm state.
	bool success = _setPwm(_switchValue.state.dimmer);
	if (success && _switchValue.state.dimmer != 0 && _switchValue.state.relay == 1) {
		// Don't use relayOff(), as that checks for switchLocked.
		switch_state_t oldVal = _switchValue;
		_relayOff();
		storeState(oldVal);
	}
	event_t event(CS_TYPE::EVT_DIMMER_POWERED, &_pwmPowered, sizeof(_pwmPowered));
	EventDispatcher::getInstance().dispatch(event);
}


/**
 * Initialize the "switch". The switch class encapsulates both the relay for switching high-power loads and the
 * IGBTs for very fast switching of low-power loads. The latter enables all types of dimming. The following types
 * of dimming exists:
 *   + pulse width modulation (PWM)
 *   + leading edge dimming
 *   + trailing edge dimming
 */
void Switch::init(const boards_config_t& board) {

	LOGd(FMT_INIT, "switch");

	PWM& pwm = PWM::getInstance();
	TYPIFY(CONFIG_PWM_PERIOD) pwmPeriod;
	State::getInstance().get(CS_TYPE::CONFIG_PWM_PERIOD, &pwmPeriod, sizeof(pwmPeriod));
	LOGd("PWM period %i pin %d", pwmPeriod, board.pinGpioPwm);

	pwm_config_t pwmConfig;
	pwmConfig.channelCount = 1;
	pwmConfig.period_us = pwmPeriod;
	pwmConfig.channels[0].pin = board.pinGpioPwm;
	pwmConfig.channels[0].inverted = board.flags.pwmInverted;
	pwm.init(pwmConfig);

	_hardwareBoard = board.hardwareBoard;

	_pinEnableDimmer = board.pinGpioEnablePwm;
	_hasRelay = board.flags.hasRelay;
	if (_hasRelay) {
		_pinRelayOff = board.pinGpioRelayOff;
		_pinRelayOn = board.pinGpioRelayOn;

		State::getInstance().get(CS_TYPE::CONFIG_RELAY_HIGH_DURATION, &_relayHighDuration, sizeof(_relayHighDuration));

		nrf_gpio_cfg_output(_pinRelayOff);
		nrf_gpio_pin_clear(_pinRelayOff);
		nrf_gpio_cfg_output(_pinRelayOn);
		nrf_gpio_pin_clear(_pinRelayOn);
	}

	// Retrieve last switch state from persistent storage
	State::getInstance().get(CS_TYPE::STATE_SWITCH_STATE, &_switchValue, sizeof(_switchValue));
	LOGd("Obtained last switch state: pwm=%u relay=%u", _switchValue.state.dimmer, _switchValue.state.relay);

	EventDispatcher::getInstance().addListener(this);
	Timer::getInstance().createSingleShot(_switchTimerId,
			(app_timer_timeout_handler_t)Switch::staticTimedSwitch);
}


void Switch::startPwm() {
	if (_pwmPowered) {
		return;
	}
	LOGd("dimmer powered");
	_pwmPowered = true;
	nrf_gpio_pin_set(_pinEnableDimmer);

	// Restore the pwm state.
	bool success = _setPwm(_switchValue.state.dimmer);
	if (success && _switchValue.state.dimmer != 0 && _switchValue.state.relay == 1) {
		// Don't use relayOff(), as that checks for switchLocked.
		switch_state_t oldVal = _switchValue;
		_relayOff();
		storeState(oldVal);
	}
	event_t event(CS_TYPE::EVT_DIMMER_POWERED, &_pwmPowered, sizeof(_pwmPowered));
	EventDispatcher::getInstance().dispatch(event);
}
