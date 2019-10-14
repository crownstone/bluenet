/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 9 Mar., 2016
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "drivers/cs_PWM.h"
#include "util/cs_BleError.h"
#include "drivers/cs_Serial.h"
#include "cfg/cs_Strings.h"
#include "protocol/cs_ErrorCodes.h"
#include "third/optmed.h"
#include <drivers/cs_RTC.h>
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
#define SECONDARY_CHANNEL_IDX       4
#define SECONDARY_CAPTURE_TASK      NRF_TIMER_TASK_CAPTURE4


// Timer channel to capture the timer counter at the zero crossing.
#define ZERO_CROSSING_CHANNEL_IDX   3
#define ZERO_CROSSING_CAPTURE_TASK  NRF_TIMER_TASK_CAPTURE3

// Define test pin to enable gpio debug.
#define PWM_DEBUG_PIN_ZERO_CROSSING_INT 15
#define PWM_DEBUG_PIN_TIMER_INT 16
#define TEST_PIN 18

PWM::PWM() :
		_initialized(false),
		_started(false),
		_startOnZeroCrossing(false)
		{
#if PWM_ENABLE==1

#endif
}

uint32_t PWM::init(const pwm_config_t& config) {
#if PWM_ENABLE==1
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

	_ppiTransitionChannels[0] = getPpiChannel(CS_PWM_PPI_CHANNEL_START + _config.channelCount*2);
	_ppiTransitionChannels[1] = getPpiChannel(CS_PWM_PPI_CHANNEL_START + _config.channelCount*2 + 1);
	_ppiGroup = getPpiGroup(CS_PWM_PPI_GROUP_START);

	// Enable timer interrupt
	err_code = sd_nvic_SetPriority(CS_PWM_IRQn, CS_PWM_TIMER_IRQ_PRIORITY);
	APP_ERROR_CHECK(err_code);
	err_code = sd_nvic_EnableIRQ(CS_PWM_IRQn);
	APP_ERROR_CHECK(err_code);

	// Init zero crossing variables
	_zeroCrossingCounter = 0;
	_adjustedMaxTickVal = _maxTickVal;
	_freqSyncedMaxTickVal = _maxTickVal;
	_zeroCrossOffsetIntegral = 0;
	_numSyncs = 0;
	_syncFrequency = true;

#ifdef PWM_DEBUG_PIN_ZERO_CROSSING_INT
    nrf_gpio_cfg_output(PWM_DEBUG_PIN_ZERO_CROSSING_INT);
#endif
#ifdef PWM_DEBUG_PIN_TIMER_INT
    nrf_gpio_cfg_output(PWM_DEBUG_PIN_TIMER_INT);
#endif
#ifdef PWM_DEBUG_PIN_TIMER_EVT
    nrf_gpio_cfg_output(PWM_DEBUG_PIN_TIMER_EVT);
#endif

    _initialized = true;

    EventDispatcher::getInstance().addListener(this);

    return ERR_SUCCESS;
#endif

    EventDispatcher::getInstance().addListener(this);
    return ERR_PWM_NOT_ENABLED;
}

uint32_t PWM::initChannel(uint8_t channel, pwm_channel_config_t& config) {
	LOGd("Configure channel %u as pin %u", channel, _config.channels[channel].pin);

	// Start off
//	nrf_gpio_cfg_output(_config.channels[i].pin);
	nrf_gpio_pin_write(_config.channels[channel].pin, _config.channels[channel].inverted ? 1 : 0);
	_values[channel] = 0;
	_nextValues[channel] = 0;

	// Configure gpiote
	_gpioteInitStatesOn[channel] = config.inverted ? NRF_GPIOTE_INITIAL_VALUE_LOW : NRF_GPIOTE_INITIAL_VALUE_HIGH;
	_gpioteInitStatesOff[channel] = config.inverted ? NRF_GPIOTE_INITIAL_VALUE_HIGH : NRF_GPIOTE_INITIAL_VALUE_LOW;

	nrf_gpiote_task_configure(CS_PWM_GPIOTE_CHANNEL_START + channel, config.pin, NRF_GPIOTE_POLARITY_TOGGLE, _gpioteInitStatesOn[channel]);

	// Cache ppi channels and gpiote tasks
	_ppiChannels[channel*2] = getPpiChannel(CS_PWM_PPI_CHANNEL_START + channel*2);
	_ppiChannels[channel*2 + 1] = getPpiChannel(CS_PWM_PPI_CHANNEL_START + channel*2 + 1);

	// Make the timer compare event trigger the gpiote task
	nrf_ppi_channel_endpoint_setup(
			_ppiChannels[channel*2],
			(uint32_t)nrf_timer_event_address_get(CS_PWM_TIMER, nrf_timer_compare_event_get(channel)),
			nrf_gpiote_task_addr_get(getGpioteTaskOut(CS_PWM_GPIOTE_CHANNEL_START + channel))
	);
	nrf_ppi_channel_endpoint_setup(
			_ppiChannels[channel*2 + 1],
			(uint32_t)nrf_timer_event_address_get(CS_PWM_TIMER, nrf_timer_compare_event_get(PERIOD_CHANNEL_IDX)),
			nrf_gpiote_task_addr_get(getGpioteTaskOut(CS_PWM_GPIOTE_CHANNEL_START + channel))
	);

//	// Enable ppi
//	nrf_ppi_channel_enable(_ppiChannels[channel*2]);
//	nrf_ppi_channel_enable(_ppiChannels[channel*2 + 1]);

	// Enable gpiote
	nrf_gpiote_task_force(CS_PWM_GPIOTE_CHANNEL_START + channel, _gpioteInitStatesOff[channel]);
	nrf_gpiote_task_enable(CS_PWM_GPIOTE_CHANNEL_START + channel);

	return 0;
}


uint32_t PWM::deinit() {

#if PWM_ENABLE==1

	LOGi("DeInit PWM");

	_initialized = false;
	return ERR_SUCCESS;
#endif

	return ERR_PWM_NOT_ENABLED;
}

uint32_t PWM::start(bool onZeroCrossing) {
	LOGi("Start");
	_startOnZeroCrossing = onZeroCrossing;
	if (!onZeroCrossing) {
		start();
	}
	return ERR_SUCCESS;
}

static void staticZeroCrossingStart(void* p_data, uint16_t len) {
	PWM::getInstance()._zeroCrossingStart();
}

void PWM::start() {
#ifdef TEST_PIN
	nrf_gpio_pin_toggle(TEST_PIN);
#endif

	// Start the timer as soon as possible.
	nrf_timer_task_trigger(CS_PWM_TIMER, NRF_TIMER_TASK_START);

	// Mark as started, else the next zero crossing calls start() again.
	// Also have to mark as started before setValue()
	_started = true;

	if (_startOnZeroCrossing) {
		// Decouple from zero crossing interrupt
		uint32_t errorCode = app_sched_event_put(NULL, 0, staticZeroCrossingStart);
		APP_ERROR_CHECK(errorCode);
	}
	else {
		LOGi("Started");
	}
}

void PWM::_zeroCrossingStart() {
	// Set all values
	// TODO: currently this only works for 1 channel! (as it calls setValue too often)
//	for (uint8_t i=0; i<_config.channelCount; ++i) {
	for (uint8_t i=0; i<1; ++i) {
		if (_values[i] != _nextValues[i]) {
			setValue(i, _nextValues[i]);
		}
	}
	LOGi("Started");
}

void PWM::setValue(uint8_t channel, uint16_t newValue) {
	if (!_initialized) {
		LOGe(FMT_NOT_INITIALIZED, "PWM");
		return;
	}
	if (!_started) {
		LOGw("Not started yet");
		// Remember what value was set, set it on start.
		_nextValues[channel] = newValue;
		return;
	}

	if (newValue > 100) {
		newValue = 100;
	}
	LOGd("Set PWM channel %d to %d", channel, newValue);
	uint32_t oldValue = _values[channel];

	switch (_values[channel]) {
	case 100:
	case 0: {
		if (newValue == 0 || newValue == 100) {
			// 0/100 --> 0/100
			gpioteForce(channel, newValue == 100);
		}
		else {
			// 0/100 --> N
			_tickValues[channel] = _maxTickVal * newValue / 100;

			nrf_ppi_channel_disable(_ppiTransitionChannels[0]);
			nrf_ppi_channel_disable(_ppiTransitionChannels[1]);
			writeCC(channel, _tickValues[channel]);

			// Turn on PPI channels at end of period (when currently on) or at trailing edge (when currently off).
			// This makes a difference, because of the initial state of the pin.
			nrf_ppi_channel_group_clear(_ppiGroup);
			nrf_ppi_channel_include_in_group(_ppiChannels[channel*2],     _ppiGroup);
			nrf_ppi_channel_include_in_group(_ppiChannels[channel*2 + 1], _ppiGroup);

			nrf_ppi_channel_endpoint_setup(
					_ppiTransitionChannels[0],
					(uint32_t)nrf_timer_event_address_get(CS_PWM_TIMER, nrf_timer_compare_event_get(oldValue == 0 ? channel : PERIOD_CHANNEL_IDX)),
					(uint32_t)nrf_ppi_task_address_get(getPpiTaskEnable(CS_PWM_PPI_GROUP_START))
			);

			nrf_ppi_channel_enable(_ppiTransitionChannels[0]);
		}
		break;
	}
	default: {
		if (newValue == 0 || newValue == 100) {
			// N --> 0/100

			nrf_ppi_channel_disable(_ppiTransitionChannels[0]);
			nrf_ppi_channel_disable(_ppiTransitionChannels[1]);
			writeCC(channel, _tickValues[channel]);

			// Turn off PPI channels at end of period (when turning on) or at trailing edge (when turning off).
			// This makes a difference, because of the initial state of the pin.
			nrf_ppi_channel_group_clear(_ppiGroup);
			nrf_ppi_channel_include_in_group(_ppiChannels[channel*2],     _ppiGroup);
			nrf_ppi_channel_include_in_group(_ppiChannels[channel*2 + 1], _ppiGroup);

			nrf_ppi_channel_endpoint_setup(
					_ppiTransitionChannels[0],
					(uint32_t)nrf_timer_event_address_get(CS_PWM_TIMER, nrf_timer_compare_event_get(newValue == 0 ? channel : PERIOD_CHANNEL_IDX)),
					(uint32_t)nrf_ppi_task_address_get(getPpiTaskDisable(CS_PWM_PPI_GROUP_START))
			);

			nrf_ppi_channel_enable(_ppiTransitionChannels[0]);
		}
		else {
			// N --> M
			_tickValues[channel] = _maxTickVal * newValue / 100;

			nrf_ppi_channel_disable(_ppiTransitionChannels[0]);
			nrf_ppi_channel_disable(_ppiTransitionChannels[1]);
			nrf_ppi_channel_group_clear(_ppiGroup);
			nrf_ppi_channel_include_in_group(_ppiTransitionChannels[0], _ppiGroup);

			// Next time the secondary channel triggers (at the new tick value), it will write this tick value to the channel CC.
			nrf_ppi_channel_endpoint_setup(
					_ppiTransitionChannels[0],
					(uint32_t)nrf_timer_event_address_get(CS_PWM_TIMER, nrf_timer_compare_event_get(SECONDARY_CHANNEL_IDX)),
					(uint32_t)nrf_timer_task_address_get(CS_PWM_TIMER, nrf_timer_capture_task_get(channel))
			);

			if (newValue < oldValue) {
				// If the new value is lower, an extra gpio task has to be setup, as the old one will be skipped.
				nrf_ppi_channel_endpoint_setup(
						_ppiTransitionChannels[1],
						(uint32_t)nrf_timer_event_address_get(CS_PWM_TIMER, nrf_timer_compare_event_get(SECONDARY_CHANNEL_IDX)),
						nrf_gpiote_task_addr_get(getGpioteTaskOut(CS_PWM_GPIOTE_CHANNEL_START + channel))
				);
				nrf_ppi_channel_include_in_group(_ppiTransitionChannels[1], _ppiGroup);
			}

			writeCC(SECONDARY_CHANNEL_IDX, _tickValues[channel]);
			nrf_ppi_group_enable(_ppiGroup);

			// TODO: the secondary channel and the ppi transition channels are never stopped/cleared (if pwm value isn't changed)?
		}
	}
	}
	_values[channel] = newValue;
}

uint16_t PWM::getValue(uint8_t channel) {
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

bool PWM::isInterruptMissed(){

	uint32_t rtcCurTime = RTC::getCount();
	uint32_t timeBetweenInterrupts = RTC::difference(rtcCurTime, _rtcTimerVal);
	_rtcTimerVal = rtcCurTime;

	if (timeBetweenInterrupts > RTC::msToTicks(21) || timeBetweenInterrupts < RTC::msToTicks(19)){
		// If there is a significant deviation from the expected difference between two interrupts, there was probably a missed interrupt.
		return true;
	}

	return false;
}

void PWM::onZeroCrossing() {

	// TODO: Handle "jumps" in the zero crossing error values brought about by unforeseen delays in the ADC

	// Capture timer value as soon as possible.
	nrf_timer_task_trigger(CS_PWM_TIMER, ZERO_CROSSING_CAPTURE_TASK);

#ifdef PWM_DEBUG_PIN_ZERO_CROSSING_INT
	nrf_gpio_pin_toggle(PWM_DEBUG_PIN_ZERO_CROSSING_INT);
#endif
	if (!_initialized) {
		LOGe(FMT_NOT_INITIALIZED, "PWM");
		return;
	}

	if (!_started) {
		if (_startOnZeroCrossing) {
			start();
		}
#ifdef PWM_DEBUG_PIN_ZERO_CROSSING_INT
	nrf_gpio_pin_toggle(PWM_DEBUG_PIN_ZERO_CROSSING_INT);
#endif

		return;
	}

	_skipZeroCrossingEvent = false;

	if (isInterruptMissed()){
		_skipZeroCrossingEvent = true;

		// There could be calculation errors introduced, so there's no point storing this interrupt's offset
		return;
	}

	_currTicks = nrf_timer_cc_read(CS_PWM_TIMER, getTimerChannel(ZERO_CROSSING_CHANNEL_IDX));

	int32_t targetTicks = 0;
	int32_t errTicks = _currTicks - targetTicks;

	// Correct error for wrap around.
	int32_t maxTickVal = _adjustedMaxTickVal;
	wrapAround(errTicks, maxTickVal);

	// cs_write("ticks=%u err=%i \r\n", _currTicks, errTicks);

	// Store error.
	_offsets[_zeroCrossingCounter] = errTicks;
}

/*
 * This method is needed as a wrapper over NRF SDK's implementation since
 * the latter does not take negative values into consideration
 */
int32_t PWM::convert_us_to_ticks(int32_t time_us){

	// Nordic SDK's API expects an unsigned int for the time parameter.
	// In case we need to convert a negative value, just multiply it by the no of ticks for 1 us

	if (time_us >= 0){
		return nrf_timer_us_to_ticks(time_us, CS_PWM_TIMER_FREQ);
	}

	// Could also be:
	// return -nrf_timer_us_to_ticks(-time_us, CS_PWM_TIMER_FREQ);
	// But the implementation below makes the process more explicit.

	int32_t one_us_in_ticks = nrf_timer_us_to_ticks(1, CS_PWM_TIMER_FREQ);
	return one_us_in_ticks*time_us;
}

/**
 * Sync the mains frequency perceived by the PWM driver coarsely to be close to the actual mains supply frequency
 */
void PWM::performCoarseFrequencySyncing(){

	if (_zeroCrossingCounter < DIMMER_NUM_CROSSINGS_PER_SLOPE_ESTIMATE) {
		return;
	}
	_zeroCrossingCounter = 0;

	// Correct error for wrap around.
	int32_t maxTickVal = _freqSyncedMaxTickVal;

	// https://en.wikipedia.org/wiki/Repeated_median_regression
	// This way we only need to calculate the median of (DIMMER_NUM_SLOPE_ESTIMATES_FOR_FREQUENCY_SYNC - 1) values,
	// but have to do that DIMMER_NUM_SLOPE_ESTIMATES_FOR_FREQUENCY_SYNC times.
	int k = 0;
	for (int i=0; i<DIMMER_NUM_CROSSINGS_PER_SLOPE_ESTIMATE; ++i) {
		int n = 0;
		for (int j=0; j<DIMMER_NUM_CROSSINGS_PER_SLOPE_ESTIMATE; ++j) {
			if (i == j) {
				continue;
			}
			int32_t dy = _offsets[j] - _offsets[i];
			wrapAround(dy, maxTickVal);
			int32_t slope = dy / (j - i);
			_offsetSlopes[n] = slope;
			++n;
		}
		_offsetSlopes2[k] = opt_med6(_offsetSlopes);
		++k;
	}
	_offsetSlopes3[_numSyncs] = opt_med7(_offsetSlopes2);

	++_numSyncs;
	if (_numSyncs < DIMMER_NUM_SLOPE_ESTIMATES_FOR_FREQUENCY_SYNC) {
		return;
	}
	_numSyncs = 0;
	int32_t medianSlope = medianSlopeMedian(_offsetSlopes3);


	// cs_write("slope=%i \r\n", medianSlope);

	// Every full cycle (~20ms), the offset increases by slope.
	// So the maxTickVal (half cycle, ~10ms) should be increased by half the slope.
	// BE CAREFUL: Changing the `_adjustedMaxTickVal` variable means that the timer compare value will change
	_adjustedMaxTickVal += medianSlope / 2;

	// Make sure the minimum max ticks > 0.99 * _maxTickVal, else dimming at 99% won't work anymore.
	uint32_t minMaxTickVal = _maxTickVal * 99 / 100 + 1;
	if (_adjustedMaxTickVal < minMaxTickVal) {
		_adjustedMaxTickVal = minMaxTickVal;
	}

	// Store frequency synchronized max ticks.
	_freqSyncedMaxTickVal = _adjustedMaxTickVal;

	// Done with frequency synchronization.
	_syncFrequency = false;

	// cs_write("slope=%i ticks=%u \r\n", medianSlope, _adjustedMaxTickVal);

	// Set the new period time at the end of the current period.
	enableInterrupt();
}

/*
 * This method is called based on the assumption that the PWM driver has a coarse estimate of the mains supply
 * frequency and it only needs to apply minor corrections to the switch timing to be within a reasonable sync
 * threshold of the zero crossings. It is essentially an estimation of the next zero crossing time based on:
 * (i) The known mains supply frequency, and (ii) a history of errors between predicted and actual zero crossing values
 */
void PWM::performPWMTimerCorrections(){

	int32_t errTicks =  _offsets[_zeroCrossingCounter];
	int32_t maxTickVal = _adjustedMaxTickVal;

	// Integrate error, but limit the integrated error (to prevent overshoot).
	// Careful that this doesn't overflow.
	_zeroCrossOffsetIntegral += errTicks;

	if (_zeroCrossingCounter < DIMMER_NUM_CROSSINGS_FOR_START_SYNC) {
		return;
	}

	_zeroCrossingCounter = 0;

	// Bound integral
	int32_t integralAbsMax = maxTickVal * DIMMER_NUM_CROSSINGS_FOR_START_SYNC * 100;
	if (_zeroCrossOffsetIntegral > integralAbsMax) {
		_zeroCrossOffsetIntegral = integralAbsMax;
	}
	if (_zeroCrossOffsetIntegral < -integralAbsMax) {
		_zeroCrossOffsetIntegral = -integralAbsMax;
	}

	int32_t medianError = errorMedian(_offsets);

	// Proportional part
	int32_t deltaP = medianError * 1800 / maxTickVal;

	// Add an integral part to the delta.
	int32_t deltaI = _zeroCrossOffsetIntegral * 2 / DIMMER_NUM_CROSSINGS_FOR_START_SYNC / maxTickVal;

	int32_t delta = deltaP + deltaI;

	// Limit the delta.
	int32_t limitDelta = maxTickVal / 200;
	if (delta > limitDelta) {
		// cs_write("LIMIT, U\r\n");
		delta = limitDelta;
	}
	if (delta < -limitDelta) {
		// cs_write("LIMIT, L\r\n");
		delta = -limitDelta;
	}

	_adjustedMaxTickVal = _freqSyncedMaxTickVal + delta;

	// Make sure the minimum max ticks > 0.99 * _maxTickVal, else dimming at 99% won't work anymore.
	uint32_t minMaxTickVal = _maxTickVal * 99 / 100 + 1;
	if (_adjustedMaxTickVal < minMaxTickVal) {
		_adjustedMaxTickVal = minMaxTickVal;
	}

	++_numSyncs;

	if (_numSyncs == DIMMER_NUM_START_SYNCS_BETWEEN_FREQ_SYNC) {
		_zeroCrossOffsetIntegral = 0;
		_numSyncs = 0;

		// TODO: Evaluate whether just setting this flag to true is OK,
		// or if the synced frequency value needs to be reset to the nominal frequency value
		_syncFrequency = true;
	}

	// cs_write("medErr=%i errInt=%i P=%i I=%i ticks=%u \r\n",  medianError, _zeroCrossOffsetIntegral, deltaP, deltaI, _adjustedMaxTickVal);

	// Set the new period time at the end of the current period.
	enableInterrupt();
}

/**
 * This callback is invoked via an event raised by the ADC driver when it is done sampling. At this point, we know
 * reasonably accurately the time of the last zero crossing interrupt and hence we can perform calculations to predict
 * the time at which the next zero crossing would occur.
 */
void PWM::onZeroCrossingTimeOffset(int32_t offset) {

	// No point in compensating for an offset that does not exist in the records.
	if (_skipZeroCrossingEvent){
		// cs_write("SKIPPING EVENT!! \r\n");
		return;
	}

	int32_t offsetInTicks = convert_us_to_ticks(offset);

	// Although it might happen that we subtract from the wrong entry, it should improve the measurements.
	// TODO: at the last sample, the counter was set to 0, making it not being compensated by the offset.

	if (_zeroCrossingCounter >= 0) {

		_offsets[_zeroCrossingCounter] -= offsetInTicks;
		 ++_zeroCrossingCounter;
	}

	if (_syncFrequency) {
		// Coarse sync to the mains supply frequency
		performCoarseFrequencySyncing();
	}
	else {
		// Finer adjustments to the PWM zero crossing prediction
		performPWMTimerCorrections();
	}
}

/*
 * The PWM driver expects events from the ADC driver for performing zero crossing offset calculations
 */
void PWM::handleEvent(event_t & event) {
	switch (event.type) {
	case (CS_TYPE::EVT_ZERO_CROSSING_TIME_OFFSET) : {
		int32_t offset = *(TYPIFY(EVT_ZERO_CROSSING_TIME_OFFSET)*)event.data;
		// cs_write("off=%i \r\n", offset);
		onZeroCrossingTimeOffset(offset);

		break;
		}
	case (CS_TYPE::EVT_ADC_RESTARTED) : {

		// cs_write("ADC RESTART!\r\n");
		break;
	}

	default : {}
	}
}

/////////////////////////////////////////
//          Private functions          //
/////////////////////////////////////////



void PWM::enableInterrupt() {
#ifdef TEST_PIN
	nrf_gpio_pin_toggle(TEST_PIN);
#endif

	// The order of the function calls below are important:
	// If we enable short stop first, the event might happen right after that,
	//   stopping the timer, and not generating a new event for the interrupt to trigger.

	// First clear the compare event, since it's still set from the last end of period event.
	nrf_timer_event_clear(CS_PWM_TIMER, nrf_timer_compare_event_get(PERIOD_CHANNEL_IDX));

//	// Make the timer stop on end of period (it will be started again in the interrupt handler).
//	nrf_timer_shorts_enable(CS_PWM_TIMER, PERIOD_SHORT_STOP_MASK);

	// Enable interrupt, set the new period value in there (at the end/start of the period).
	nrf_timer_int_enable(CS_PWM_TIMER, nrf_timer_compare_int_get(PERIOD_CHANNEL_IDX));
}

void PWM::_handleInterrupt() {
	// At this point, the timer counter is close to 0.
	// So we have about 10ms before the period compare event triggers again.
	// This should be plenty of time to change the period value before it gets triggered.

	// Set the new period value.
	writeCC(PERIOD_CHANNEL_IDX, _adjustedMaxTickVal);

//	// Don't stop timer on end of period anymore, and start the timer again
//	nrf_timer_shorts_disable(CS_PWM_TIMER, PERIOD_SHORT_STOP_MASK);
//	nrf_timer_task_trigger(CS_PWM_TIMER, NRF_TIMER_TASK_START);
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

void PWM::wrapAround(int32_t& val, int32_t max) {
	val = (val + max / 2) % max - max / 2;

	// Same operation, different logic
	/*
	if (val > max / 2) {
		val -= max;
	}
	else if (val < -max / 2) {
		val += max;
	}
	*/
}

nrf_timer_cc_channel_t PWM::getTimerChannel(uint8_t index) {
	assert(index < 6, "invalid timer channel index");
	switch(index) {
	case 0:
		return NRF_TIMER_CC_CHANNEL0;
	case 1:
		return NRF_TIMER_CC_CHANNEL1;
	case 2:
		return NRF_TIMER_CC_CHANNEL2;
	case 3:
		return NRF_TIMER_CC_CHANNEL3;
	case 4:
		return NRF_TIMER_CC_CHANNEL4;
	case 5:
		return NRF_TIMER_CC_CHANNEL5;
	}
	APP_ERROR_CHECK(NRF_ERROR_INVALID_PARAM);
	return NRF_TIMER_CC_CHANNEL0;
}

nrf_gpiote_tasks_t PWM::getGpioteTaskOut(uint8_t index) {
	assert(index < 8, "invalid gpiote task index");
	switch(index) {
	case 0:
		return NRF_GPIOTE_TASKS_OUT_0;
	case 1:
		return NRF_GPIOTE_TASKS_OUT_1;
	case 2:
		return NRF_GPIOTE_TASKS_OUT_2;
	case 3:
		return NRF_GPIOTE_TASKS_OUT_3;
	case 4:
		return NRF_GPIOTE_TASKS_OUT_4;
	case 5:
		return NRF_GPIOTE_TASKS_OUT_5;
	case 6:
		return NRF_GPIOTE_TASKS_OUT_6;
	case 7:
		return NRF_GPIOTE_TASKS_OUT_7;
	}
	APP_ERROR_CHECK(NRF_ERROR_INVALID_PARAM);
	return NRF_GPIOTE_TASKS_OUT_0;
}

nrf_ppi_channel_t PWM::getPpiChannel(uint8_t index) {
	assert(index < 16, "invalid ppi channel index");
	switch(index) {
	case 0:
		return NRF_PPI_CHANNEL0;
	case 1:
		return NRF_PPI_CHANNEL1;
	case 2:
		return NRF_PPI_CHANNEL2;
	case 3:
		return NRF_PPI_CHANNEL3;
	case 4:
		return NRF_PPI_CHANNEL4;
	case 5:
		return NRF_PPI_CHANNEL5;
	case 6:
		return NRF_PPI_CHANNEL6;
	case 7:
		return NRF_PPI_CHANNEL7;
	case 8:
		return NRF_PPI_CHANNEL8;
	case 9:
		return NRF_PPI_CHANNEL9;
	case 10:
		return NRF_PPI_CHANNEL10;
	case 11:
		return NRF_PPI_CHANNEL11;
	case 12:
		return NRF_PPI_CHANNEL12;
	case 13:
		return NRF_PPI_CHANNEL13;
	case 14:
		return NRF_PPI_CHANNEL14;
	case 15:
		return NRF_PPI_CHANNEL15;
	}
	return NRF_PPI_CHANNEL0;
}

nrf_ppi_channel_group_t PWM::getPpiGroup(uint8_t index) {
	assert(index < 6, "invalid ppi group index");
	switch(index) {
	case 0:
		return NRF_PPI_CHANNEL_GROUP0;
	case 1:
		return NRF_PPI_CHANNEL_GROUP1;
	case 2:
		return NRF_PPI_CHANNEL_GROUP2;
	case 3:
		return NRF_PPI_CHANNEL_GROUP3;
	case 4:
		return NRF_PPI_CHANNEL_GROUP4;
	case 5:
		return NRF_PPI_CHANNEL_GROUP5;
	}
	return NRF_PPI_CHANNEL_GROUP0;
}

nrf_ppi_task_t PWM::getPpiTaskEnable(uint8_t index) {
	assert(index < 6, "invalid ppi group index");
	switch(index) {
	case 0:
		return NRF_PPI_TASK_CHG0_EN;
	case 1:
		return NRF_PPI_TASK_CHG1_EN;
	case 2:
		return NRF_PPI_TASK_CHG2_EN;
	case 3:
		return NRF_PPI_TASK_CHG3_EN;
	case 4:
		return NRF_PPI_TASK_CHG4_EN;
	case 5:
		return NRF_PPI_TASK_CHG5_EN;
	}
	return NRF_PPI_TASK_CHG0_EN;
}

nrf_ppi_task_t PWM::getPpiTaskDisable(uint8_t index) {
	assert(index < 6, "invalid ppi group index");
	switch(index) {
	case 0:
		return NRF_PPI_TASK_CHG0_DIS;
	case 1:
		return NRF_PPI_TASK_CHG1_DIS;
	case 2:
		return NRF_PPI_TASK_CHG2_DIS;
	case 3:
		return NRF_PPI_TASK_CHG3_DIS;
	case 4:
		return NRF_PPI_TASK_CHG4_DIS;
	case 5:
		return NRF_PPI_TASK_CHG5_DIS;
	}
	return NRF_PPI_TASK_CHG0_DIS;
}


// Timer interrupt handler
extern "C" void CS_PWM_TIMER_IRQ(void) {
#ifdef TEST_PIN
	nrf_gpio_pin_toggle(TEST_PIN);
#endif
//	if (nrf_timer_event_check(CS_PWM_TIMER, nrf_timer_compare_event_get(PERIOD_CHANNEL_IDX))) {
	// Clear and disable interrupt until next change.
	nrf_timer_int_disable(CS_PWM_TIMER, nrf_timer_compare_int_get(PERIOD_CHANNEL_IDX));
	PWM::getInstance()._handleInterrupt();
//	}
}
