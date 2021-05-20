/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 9 Mar., 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "drivers/cs_PWM.h"
#include "util/cs_BleError.h"
#include <logging/cs_Logger.h>
#include "cfg/cs_Strings.h"
#include "protocol/cs_ErrorCodes.h"

#include <nrf.h>
#include <app_util_platform.h>

/**********************************************************************************************************************
 * Pulse Width Modulation class
 *********************************************************************************************************************/

// Timer channel that determines the period is set to the last channel available.
#define PERIOD_CHANNEL_IDX          5
#define PERIOD_SHORT_CLEAR_MASK     NRF_TIMER_SHORT_COMPARE5_CLEAR_MASK
#define PERIOD_SHORT_STOP_MASK      NRF_TIMER_SHORT_COMPARE5_STOP_MASK

// Timer channel that is used when duty cycle is changed.
#define TRANSITION_CHANNEL_IDX      4
#define TRANSITION_CAPTURE_TASK     NRF_TIMER_TASK_CAPTURE4


// Timer channel to capture the timer counter at the zero crossing.
#define ZERO_CROSSING_CHANNEL_IDX   3
#define ZERO_CROSSING_CAPTURE_TASK  NRF_TIMER_TASK_CAPTURE3

// Set to true to enable gpio debug.
#define PWM_GPIO_DEBUG false

#if (PWM_GPIO_DEBUG == true)
	#ifdef DEBUG
		#pragma message("PWM test pin enabled")
	#else
		#warning "PWM test pin enabled"
	#endif
	#define PWM_TEST_PIN 20
	#define PWM_TEST_PIN_INIT nrf_gpio_cfg_output(PWM_TEST_PIN)
	#define PWM_TEST_PIN_TOGGLE nrf_gpio_pin_toggle(PWM_TEST_PIN)
#else
	#define PWM_TEST_PIN_INIT
	#define PWM_TEST_PIN_TOGGLE
#endif



#define LOGPwmDebug LOGnone

PWM::PWM() :
		_initialized(false),
		_started(false),
		_startOnZeroCrossing(false)
		{
}

uint32_t PWM::init(const pwm_config_t& config) {
	LOGi(FMT_INIT, "PWM");
	_config = config;
//	memcpy(&_config, &config, sizeof(pwm_config_t));

	uint32_t err_code;

	// Setup timer
	nrf_timer_task_trigger(CS_PWM_TIMER, NRF_TIMER_TASK_CLEAR);
	nrf_timer_bit_width_set(CS_PWM_TIMER, NRF_TIMER_BIT_WIDTH_32);
//	// Using 16 bit width is a dirty fix for sometimes missing the period compare event. With a 16bit timer it overflows quicker.
//	nrf_timer_bit_width_set(CS_PWM_TIMER, NRF_TIMER_BIT_WIDTH_16);
	nrf_timer_frequency_set(CS_PWM_TIMER, CS_PWM_TIMER_FREQ);
	_maxTickVal = nrf_timer_us_to_ticks(_config.period_us, CS_PWM_TIMER_FREQ);
	LOGd("maxTicks=%u", _maxTickVal);
	nrf_timer_cc_write(CS_PWM_TIMER, getTimerChannel(PERIOD_CHANNEL_IDX), _maxTickVal);
	nrf_timer_mode_set(CS_PWM_TIMER, NRF_TIMER_MODE_TIMER);
	nrf_timer_shorts_enable(CS_PWM_TIMER, PERIOD_SHORT_CLEAR_MASK);
	nrf_timer_event_clear(CS_PWM_TIMER, nrf_timer_compare_event_get(0));
	nrf_timer_event_clear(CS_PWM_TIMER, nrf_timer_compare_event_get(1));
	nrf_timer_event_clear(CS_PWM_TIMER, nrf_timer_compare_event_get(2));
	nrf_timer_event_clear(CS_PWM_TIMER, nrf_timer_compare_event_get(3));
	nrf_timer_event_clear(CS_PWM_TIMER, nrf_timer_compare_event_get(4));
	nrf_timer_event_clear(CS_PWM_TIMER, nrf_timer_compare_event_get(5));


	// Init gpiote and ppi for each channel
	for (uint8_t i=0; i<_config.channelCount; ++i) {
		initChannel(i, _config.channels[i]);
	}

	_ppiTransitionChannel = getPpiChannel(CS_PWM_PPI_CHANNEL_START + _config.channelCount * 2);

	// Enable timer interrupt
	err_code = sd_nvic_SetPriority(CS_PWM_IRQn, CS_PWM_TIMER_IRQ_PRIORITY);
	APP_ERROR_CHECK(err_code);
	err_code = sd_nvic_EnableIRQ(CS_PWM_IRQn);
	APP_ERROR_CHECK(err_code);

	// Init zero crossing variables
	_zeroCrossingCounter = 0;
	_adjustedMaxTickVal = _maxTickVal;
	_zeroCrossDeviationIntegral = 0;
	_zeroCrossTicksDeviationAvg = 0;

	PWM_TEST_PIN_INIT;

	_initialized = true;
	return ERR_SUCCESS;
}

uint32_t PWM::initChannel(uint8_t channel, pwm_channel_config_t& config) {
	LOGd("Configure channel %u as pin %u", channel, config.pin);

	// Start off
	nrf_gpio_pin_write(config.pin, config.inverted ? 1 : 0);

	// Configure GPIOTE
	_gpioteInitStatesOn[channel] = config.inverted ? NRF_GPIOTE_INITIAL_VALUE_LOW : NRF_GPIOTE_INITIAL_VALUE_HIGH;
	_gpioteInitStatesOff[channel] = config.inverted ? NRF_GPIOTE_INITIAL_VALUE_HIGH : NRF_GPIOTE_INITIAL_VALUE_LOW;
	nrf_gpiote_task_configure(CS_PWM_GPIOTE_CHANNEL_START + channel, config.pin, NRF_GPIOTE_POLARITY_TOGGLE, _gpioteInitStatesOn[channel]);

	// Cache PPI channels
	_ppiChannelsOn[channel] = getPpiChannel(CS_PWM_PPI_CHANNEL_START + channel * 2);
	_ppiChannelsOff[channel] = getPpiChannel(CS_PWM_PPI_CHANNEL_START + channel * 2 + 1);

	// Make the timer compare event trigger the GPIOTE set and clear task:

	// At the end of the duty cycle, turn switch off.
	nrf_ppi_channel_endpoint_setup(
			_ppiChannelsOff[channel],
			(uint32_t)nrf_timer_event_address_get(CS_PWM_TIMER, nrf_timer_compare_event_get(channel)),
			nrf_gpiote_task_addr_get(config.inverted ? getGpioteTaskSet(CS_PWM_GPIOTE_CHANNEL_START + channel) : getGpioteTaskClear(CS_PWM_GPIOTE_CHANNEL_START + channel))
	);

	// At the start/end of the period, turn switch on.
	nrf_ppi_channel_endpoint_setup(
			_ppiChannelsOn[channel],
			(uint32_t)nrf_timer_event_address_get(CS_PWM_TIMER, nrf_timer_compare_event_get(PERIOD_CHANNEL_IDX)),
			nrf_gpiote_task_addr_get(config.inverted ? getGpioteTaskClear(CS_PWM_GPIOTE_CHANNEL_START + channel) : getGpioteTaskSet(CS_PWM_GPIOTE_CHANNEL_START + channel))
	);

	// Set initial CC value.
	// 0 doesn't work, that never triggers.
	writeCC(channel, 1);

	// Enable gpiote
	nrf_gpiote_task_force(CS_PWM_GPIOTE_CHANNEL_START + channel, _gpioteInitStatesOff[channel]);
	nrf_gpiote_task_enable(CS_PWM_GPIOTE_CHANNEL_START + channel);

	return 0;
}


uint32_t PWM::deinit() {
	LOGi("DeInit PWM");

	_initialized = false;
	return ERR_SUCCESS;
}

uint32_t PWM::start(bool onZeroCrossing) {
	assert(_initialized, "ERR_NOT_INITIALIZED");

	LOGi("Start. Wait for zero crossing: %u", onZeroCrossing);

	_startOnZeroCrossing = onZeroCrossing;
	if (!onZeroCrossing) {
		start();
	}
	return ERR_SUCCESS;
}

void onTimerEnd(void* p_data, uint16_t len) {
	PWM::getInstance().onPeriodEnd();
}

void PWM::onPeriodEnd() {
	updateValues();
}


void PWM::start() {
	PWM_TEST_PIN_TOGGLE;

	// Start the timer as soon as possible.
	nrf_timer_task_trigger(CS_PWM_TIMER, NRF_TIMER_TASK_START);

	// Enabled the timer interrupt on period end.
	nrf_timer_int_enable(CS_PWM_TIMER, nrf_timer_compare_int_get(PERIOD_CHANNEL_IDX));

	// Mark as started, else the next zero crossing calls start() again.
	// Also have to mark as started before setValue()
	_started = true;

	if (!_startOnZeroCrossing) {
		LOGi("Started");
	}
}

void PWM::setValue(uint8_t channel, uint8_t newValue, uint8_t stepSize) {
	if (!_initialized) {
		LOGe(FMT_NOT_INITIALIZED, "PWM");
		return;
	}

	if (channel > CS_PWM_MAX_CHANNELS) {
		LOGe("Invalid channel %u", channel);
		return;
	}

	LOGd("Set PWM channel %d to %d", channel, newValue);

	if (newValue > _maxValue) {
		newValue = _maxValue;
	}

	if (stepSize < 1) {
		stepSize = 1;
	}

	// Simply store the target value, don't actually set it,
	// that will be done by updateValues().
	_targetValues[channel] = newValue;
	_stepSize[channel] = stepSize;

	// Unless value is 0 or 100 and speed 100
	// In that case we can and should set the value immediately, as it might be for to safety.
	if ((newValue == 0 || newValue == _maxValue) && stepSize >= _maxValue) {
		setValue(channel, newValue);
	}
}

void PWM::updateValues() {
	if (!_started) {
		return;
	}

	// Only set values every N periods, to avoid setValue() being called too often.
	if (--_updateValuesCountdown != 0) {
		return;
	}
	_updateValuesCountdown = numPeriodsBeforeValueUpdate;

	for (uint8_t channel = 0; channel < CS_PWM_MAX_CHANNELS; ++channel) {
		int16_t diff = _targetValues[channel] - _values[channel];
		int16_t inc = 0;
		if (diff > 0) {
			inc = _stepSize[channel];
			if (inc > diff) {
				inc = diff;
			}
		}
		else if (diff < 0) {
			inc = -_stepSize[channel];
			if (inc < diff) {
				inc = diff;
			}
		}
		if (inc != 0) {
			// Only 1 value can be set at a time.
			setValue(channel, _values[channel] + inc);
			return;
		}
	}
}

void PWM::setValue(uint8_t channel, uint8_t newValue) {
	if (_values[channel] == newValue) {
		LOGd("Channel %u is already set to %u", channel, newValue);
		return;
	}

//	if (checkInTransition()) {
//		LOGd("Transition in progress, set value later");
//		_nextValues[channel] = newValue;
//		return;
//	}

	uint32_t oldValue = _values[channel];
	_values[channel] = newValue;

	// Something weird happens for low values: the resulting intensity is way too large.
	// Either a software bug, peripheral issue, or hardware issue.
	// Cap the value after storing it to _values, else _values will never reach 0.
	if (newValue < 10) {
		newValue = 0;
	}

	uint32_t oldTickValue = _tickValues[channel];
	_tickValues[channel] = _maxTickVal * newValue / _maxValue;
	LOGPwmDebug("Set PWM channel %u to %u ticks=%u", channel, newValue, _tickValues[channel]);

	// Always disable the temporary PPI.
	nrf_ppi_channel_disable(_ppiTransitionChannel);

	switch (newValue) {
		case 0:
			// Simply disable the PPI that turns on the switch.
			// Don't forget that the CC value has to be set.
			nrf_ppi_channel_disable(_ppiChannelsOn[channel]);
			nrf_ppi_channel_enable(_ppiChannelsOff[channel]);

			// Disable the PPI that turns on the switch, and force gpio value.
			// This turns it off immediately instead of waiting for the CC event.
			// However, this also results in 1 period with a low duty cycle, resulting in a high power output (see above).
//			nrf_ppi_channel_disable(_ppiChannelsOn[channel]);
//			gpioteForce(channel, false);

			LOGPwmDebug("ppiEnabled=%u CC=%u", NRF_PPI->CHEN, readCC(channel));
			break;
		case _maxValue:
			// Simply disable the PPI that turns off the switch.
			// This way, it turns on at the start of the period.
			nrf_ppi_channel_disable(_ppiChannelsOff[channel]);
			nrf_ppi_channel_enable(_ppiChannelsOn[channel]);

			LOGPwmDebug("ppiEnabled=%u CC=%u", NRF_PPI->CHEN, readCC(channel));
			break;
		default: {
			if (oldValue != 0 && oldValue != _maxValue && newValue < oldValue) {
				// From dimmed value to lower dimmed value.

				// Turn switch off at end of the old tick value.
				// This is required to turn off the switch in case the current timer value is higher than the new tick value, but lower than the old tick value.
				// So this PPI is only temporarily needed, until the timer reached the start of the period again.
				LOGPwmDebug("transition: turn off at %u", oldTickValue);
				writeCC(TRANSITION_CHANNEL_IDX, oldTickValue);
				nrf_ppi_channel_endpoint_setup(
						_ppiTransitionChannel,
						(uint32_t)nrf_timer_event_address_get(CS_PWM_TIMER, nrf_timer_compare_event_get(TRANSITION_CHANNEL_IDX)),
						nrf_gpiote_task_addr_get(_config.channels[channel].inverted ? getGpioteTaskSet(CS_PWM_GPIOTE_CHANNEL_START + channel) : getGpioteTaskClear(CS_PWM_GPIOTE_CHANNEL_START + channel))
				);
				nrf_ppi_channel_enable(_ppiTransitionChannel);

//				// Wait for transition to be done.
//				_transitionInProgress = true;
//				_transitionTargetTicks = _tickValues[channel];
			}
			LOGPwmDebug("writeCC %u", _tickValues[channel]);
			writeCC(channel, _tickValues[channel]);

			// Enable turn off first, else turn on might happen before the turn off ppi is enabled.
			nrf_ppi_channel_enable(_ppiChannelsOff[channel]);
			nrf_ppi_channel_enable(_ppiChannelsOn[channel]);
		}
	}
}

uint8_t PWM::getValue(uint8_t channel) {
	if (!_initialized) {
		LOGe(FMT_NOT_INITIALIZED, "PWM");
		return 0;
	}
	if (channel >= _config.channelCount) {
		LOGe("Invalid channel");
		return 0;
	}
	return _values[channel];
}

void PWM::onZeroCrossingInterrupt() {
	if (!_initialized) {
		LOGe(FMT_NOT_INITIALIZED, "PWM");
		return;
	}

	if (!_started) {
		if (_startOnZeroCrossing) {
			start();
		}
		return;
	}

#ifndef PWM_SYNC_IMMEDIATELY
	nrf_timer_task_trigger(CS_PWM_TIMER, ZERO_CROSSING_CAPTURE_TASK);
	uint32_t ticks = nrf_timer_cc_read(CS_PWM_TIMER, getTimerChannel(ZERO_CROSSING_CHANNEL_IDX));

	int64_t targetTicks = 0;
	int64_t errTicks = targetTicks - ticks;

	// Correct error for wrap around.
	int64_t maxTickVal = _maxTickVal;
//	errTicks = (errTicks + maxTickVal/2) % maxTickVal - maxTickVal/2; // Doesn't work?
	if (errTicks > maxTickVal / 2) {
		errTicks -= maxTickVal;
	}
	else if (errTicks < -maxTickVal / 2) {
		errTicks += maxTickVal;
	}

	// Integrate error, but limit the integrated error (to prevent overflow and overshoot)
	_zeroCrossDeviationIntegral += -errTicks;
	int64_t integralAbsMax = maxTickVal * 1000;
	if (_zeroCrossDeviationIntegral > integralAbsMax) {
		_zeroCrossDeviationIntegral = integralAbsMax;
	}
	if (_zeroCrossDeviationIntegral < -integralAbsMax) {
		_zeroCrossDeviationIntegral = -integralAbsMax;
	}


	// Exponential moving average
	uint32_t alpha = 1000; // 1000: no averaging
//	uint32_t alpha = 800; // Discount factor
	_zeroCrossTicksDeviationAvg = ((1000-alpha) * _zeroCrossTicksDeviationAvg + alpha * errTicks) / 1000;

	++_zeroCrossingCounter;
	if (_zeroCrossingCounter % 10 == 0) {
		// Calculate the new period value.
		int32_t delta = 0;

		// Proportional part
		int32_t deltaP = -errTicks / (maxTickVal/400);

		// Add an integral part to the delta.
		int32_t deltaI = _zeroCrossDeviationIntegral / 1000 / (maxTickVal/400);

		delta = deltaP + deltaI;
		// Limit the output, make sure the minimum newMaxTicks > 0.99 * _maxTickVal, else dimming at 99% won't work anymore.
		int32_t limitDelta = maxTickVal / 120;
		if (delta > limitDelta) {
			delta = limitDelta;
		}
		if (delta < -limitDelta) {
			delta = -limitDelta;
		}
		uint32_t newMaxTicks = maxTickVal + delta;
		_adjustedMaxTickVal = newMaxTicks;

		// Set the new period time at the end of the current period.
		enableInterrupt();

		if (ticks > _maxTickVal + maxTickVal / 120) {
			LOGe("%u  %u  %u\r\n", ticks, errTicks, newMaxTicks);
		}

		if (_zeroCrossingCounter % 50 == 0) {
//			write("%u  %lli  %lli  %lli %i %u\r\n", ticks, errTicks, _zeroCrossTicksDeviationAvg, _zeroCrossDeviationIntegral, deltaI, newMaxTicks);
		}
	}

#else
	// Start a new period
	// Need to stop the timer, else the gpio state at start is not consistent.
	// I guess this is because the gpiote toggle sometimes happens before, sometimes after the nrf_gpiote_task_force()
	nrf_timer_event_clear(CS_PWM_TIMER, nrf_timer_compare_event_get(PERIOD_CHANNEL_IDX));
	nrf_timer_task_trigger(CS_PWM_TIMER, NRF_TIMER_TASK_STOP);

	for (uint8_t i=0; i<_config.channelCount; ++i) {
		if (_isPwmEnabled[i]) {
#ifdef PWM_CENTERED
			gpioteForce(i, false);
#else
			gpioteForce(i, true);
#endif
		}
	}
	// Set the counter back to 0, and start the timer again.
	nrf_timer_task_trigger(CS_PWM_TIMER, NRF_TIMER_TASK_CLEAR);
	nrf_timer_task_trigger(CS_PWM_TIMER, NRF_TIMER_TASK_START);
#endif // ndef PWM_SYNC_IMMEDIATELY
}


/////////////////////////////////////////
//          Private functions          //
/////////////////////////////////////////



void PWM::enableInterrupt() {
	PWM_TEST_PIN_TOGGLE;

	// The order of the function calls below are important:
	// If we enable short stop first, the event might happen right after that,
	//   stopping the timer, and not generating a new event for the interrupt to trigger.

//	// First clear the compare event, since it's still set from the last end of period event.
//	nrf_timer_event_clear(CS_PWM_TIMER, nrf_timer_compare_event_get(PERIOD_CHANNEL_IDX));

//	// Make the timer stop on end of period (it will be started again in the interrupt handler).
//	nrf_timer_shorts_enable(CS_PWM_TIMER, PERIOD_SHORT_STOP_MASK);

//	// Enable interrupt, set the new period value in there (at the end/start of the period).
//	nrf_timer_int_enable(CS_PWM_TIMER, nrf_timer_compare_int_get(PERIOD_CHANNEL_IDX));
}

void PWM::_handleInterrupt() {
	// At this point, the timer counter is close to 0.
	// So we have about 10ms before the period compare event triggers again.
	// This should be plenty of time to change the period value before it gets triggered.

	// Set the new period value.
	writeCC(PERIOD_CHANNEL_IDX, _adjustedMaxTickVal);

	// Decouple from zero crossing interrupt
	uint16_t schedulerSpace = app_sched_queue_space_get();
	if (schedulerSpace > 10) {
		uint32_t errorCode = app_sched_event_put(NULL, 0, onTimerEnd);
		APP_ERROR_CHECK(errorCode);
	}

//	// Don't stop timer on end of period anymore, and start the timer again
//	nrf_timer_shorts_disable(CS_PWM_TIMER, PERIOD_SHORT_STOP_MASK);
//	nrf_timer_task_trigger(CS_PWM_TIMER, NRF_TIMER_TASK_START);

	// Clear the compare event.
	nrf_timer_event_clear(CS_PWM_TIMER, nrf_timer_compare_event_get(PERIOD_CHANNEL_IDX));
}




void PWM::gpioteConfig(uint8_t channel, bool initOn) {
	nrf_gpiote_task_configure(
			CS_PWM_GPIOTE_CHANNEL_START + channel,
			_config.channels[channel].pin,
			NRF_GPIOTE_POLARITY_TOGGLE,
			initOn ? _gpioteInitStatesOn[channel] : _gpioteInitStatesOff[channel]
	);
}

void PWM::gpioteUnconfig(uint8_t channel) {
	nrf_gpiote_te_default(CS_PWM_GPIOTE_CHANNEL_START + channel);
}

void PWM::gpioteEnable(uint8_t channel) {
	nrf_gpiote_task_enable(CS_PWM_GPIOTE_CHANNEL_START + channel);
}

void PWM::gpioteDisable(uint8_t channel) {
	nrf_gpiote_task_disable(CS_PWM_GPIOTE_CHANNEL_START + channel);
}

void PWM::gpioteForce(uint8_t channel, bool initOn) {
	nrf_gpiote_task_force(CS_PWM_GPIOTE_CHANNEL_START + channel, initOn ? _gpioteInitStatesOn[channel] : _gpioteInitStatesOff[channel]);
}

void PWM::gpioOn(uint8_t channel) {
	nrf_gpio_pin_write(_config.channels[channel].pin, _config.channels[channel].inverted ? 0 : 1);
}

void PWM::gpioOff(uint8_t channel) {
	nrf_gpio_pin_write(_config.channels[channel].pin, _config.channels[channel].inverted ? 1 : 0);
}

void PWM::writeCC(uint8_t channelIdx, uint32_t ticks) {
	nrf_timer_cc_write(CS_PWM_TIMER, getTimerChannel(channelIdx), ticks);
}

uint32_t PWM::readCC(uint8_t channelIdx) {
	return nrf_timer_cc_read(CS_PWM_TIMER, getTimerChannel(channelIdx));
}

nrf_timer_cc_channel_t PWM::getTimerChannel(uint8_t index) {
	assert(index < 6, "invalid timer channel index");
	switch (index) {
		case 0: return NRF_TIMER_CC_CHANNEL0;
		case 1: return NRF_TIMER_CC_CHANNEL1;
		case 2: return NRF_TIMER_CC_CHANNEL2;
		case 3: return NRF_TIMER_CC_CHANNEL3;
		case 4: return NRF_TIMER_CC_CHANNEL4;
		case 5: return NRF_TIMER_CC_CHANNEL5;
	}
	APP_ERROR_CHECK(NRF_ERROR_INVALID_PARAM);
	return NRF_TIMER_CC_CHANNEL0;
}

nrf_gpiote_tasks_t PWM::getGpioteTaskOut(uint8_t index) {
	assert(index < 8, "invalid gpiote task index");
	switch (index) {
		case 0: return NRF_GPIOTE_TASKS_OUT_0;
		case 1: return NRF_GPIOTE_TASKS_OUT_1;
		case 2: return NRF_GPIOTE_TASKS_OUT_2;
		case 3: return NRF_GPIOTE_TASKS_OUT_3;
		case 4: return NRF_GPIOTE_TASKS_OUT_4;
		case 5: return NRF_GPIOTE_TASKS_OUT_5;
		case 6: return NRF_GPIOTE_TASKS_OUT_6;
		case 7: return NRF_GPIOTE_TASKS_OUT_7;
	}
	APP_ERROR_CHECK(NRF_ERROR_INVALID_PARAM);
	return NRF_GPIOTE_TASKS_OUT_0;
}

nrf_gpiote_tasks_t PWM::getGpioteTaskSet(uint8_t index) {
	assert(index < 8, "invalid gpiote task index");
	switch (index) {
		case 0: return NRF_GPIOTE_TASKS_SET_0;
		case 1: return NRF_GPIOTE_TASKS_SET_1;
		case 2: return NRF_GPIOTE_TASKS_SET_2;
		case 3: return NRF_GPIOTE_TASKS_SET_3;
		case 4: return NRF_GPIOTE_TASKS_SET_4;
		case 5: return NRF_GPIOTE_TASKS_SET_5;
		case 6: return NRF_GPIOTE_TASKS_SET_6;
		case 7: return NRF_GPIOTE_TASKS_SET_7;
	}
	APP_ERROR_CHECK(NRF_ERROR_INVALID_PARAM);
	return NRF_GPIOTE_TASKS_SET_0;
}

nrf_gpiote_tasks_t PWM::getGpioteTaskClear(uint8_t index) {
	assert(index < 8, "invalid gpiote task index");
	switch (index) {
		case 0: return NRF_GPIOTE_TASKS_CLR_0;
		case 1: return NRF_GPIOTE_TASKS_CLR_1;
		case 2: return NRF_GPIOTE_TASKS_CLR_2;
		case 3: return NRF_GPIOTE_TASKS_CLR_3;
		case 4: return NRF_GPIOTE_TASKS_CLR_4;
		case 5: return NRF_GPIOTE_TASKS_CLR_5;
		case 6: return NRF_GPIOTE_TASKS_CLR_6;
		case 7: return NRF_GPIOTE_TASKS_CLR_7;
	}
	APP_ERROR_CHECK(NRF_ERROR_INVALID_PARAM);
	return NRF_GPIOTE_TASKS_CLR_0;
}

nrf_ppi_channel_t PWM::getPpiChannel(uint8_t index) {
	assert(index < 16, "invalid ppi channel index");
	switch (index) {
		case 0: return NRF_PPI_CHANNEL0;
		case 1: return NRF_PPI_CHANNEL1;
		case 2: return NRF_PPI_CHANNEL2;
		case 3: return NRF_PPI_CHANNEL3;
		case 4: return NRF_PPI_CHANNEL4;
		case 5: return NRF_PPI_CHANNEL5;
		case 6: return NRF_PPI_CHANNEL6;
		case 7: return NRF_PPI_CHANNEL7;
		case 8: return NRF_PPI_CHANNEL8;
		case 9: return NRF_PPI_CHANNEL9;
		case 10: return NRF_PPI_CHANNEL10;
		case 11: return NRF_PPI_CHANNEL11;
		case 12: return NRF_PPI_CHANNEL12;
		case 13: return NRF_PPI_CHANNEL13;
		case 14: return NRF_PPI_CHANNEL14;
		case 15: return NRF_PPI_CHANNEL15;
	}
	return NRF_PPI_CHANNEL0;
}

nrf_ppi_channel_group_t PWM::getPpiGroup(uint8_t index) {
	assert(index < 6, "invalid ppi group index");
	switch (index) {
	case 0: return NRF_PPI_CHANNEL_GROUP0;
	case 1: return NRF_PPI_CHANNEL_GROUP1;
	case 2: return NRF_PPI_CHANNEL_GROUP2;
	case 3: return NRF_PPI_CHANNEL_GROUP3;
	case 4: return NRF_PPI_CHANNEL_GROUP4;
	case 5: return NRF_PPI_CHANNEL_GROUP5;
	}
	return NRF_PPI_CHANNEL_GROUP0;
}

nrf_ppi_task_t PWM::getPpiTaskEnable(uint8_t index) {
	assert(index < 6, "invalid ppi group index");
	switch (index) {
	case 0: return NRF_PPI_TASK_CHG0_EN;
	case 1: return NRF_PPI_TASK_CHG1_EN;
	case 2: return NRF_PPI_TASK_CHG2_EN;
	case 3: return NRF_PPI_TASK_CHG3_EN;
	case 4: return NRF_PPI_TASK_CHG4_EN;
	case 5: return NRF_PPI_TASK_CHG5_EN;
	}
	return NRF_PPI_TASK_CHG0_EN;
}

nrf_ppi_task_t PWM::getPpiTaskDisable(uint8_t index) {
	assert(index < 6, "invalid ppi group index");
	switch (index) {
	case 0: return NRF_PPI_TASK_CHG0_DIS;
	case 1: return NRF_PPI_TASK_CHG1_DIS;
	case 2: return NRF_PPI_TASK_CHG2_DIS;
	case 3: return NRF_PPI_TASK_CHG3_DIS;
	case 4: return NRF_PPI_TASK_CHG4_DIS;
	case 5: return NRF_PPI_TASK_CHG5_DIS;
	}
	return NRF_PPI_TASK_CHG0_DIS;
}


// Timer interrupt handler
extern "C" void CS_PWM_TIMER_IRQ(void) {
	PWM_TEST_PIN_TOGGLE;
//	if (nrf_timer_event_check(CS_PWM_TIMER, nrf_timer_compare_event_get(PERIOD_CHANNEL_IDX))) {
//	// Clear and disable interrupt until next change.
//	nrf_timer_int_disable(CS_PWM_TIMER, nrf_timer_compare_int_get(PERIOD_CHANNEL_IDX));
	PWM::getInstance()._handleInterrupt();
//	}
}
