/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 9 Mar., 2016
 * Triple-license: LGPLv3+, Apache License, and/or MIT
 */
#pragma once

#include <cstdint>

#include "ble/cs_Nordic.h"
#include "cfg/cs_Config.h"

#define ERR_PWM_NOT_ENABLED 1


typedef struct {
	uint16_t pin;
	bool inverted;
} pwm_channel_config_t;

typedef struct {
	uint32_t period_us;
	uint8_t channelCount;
	pwm_channel_config_t channels[CS_PWM_MAX_CHANNELS];
} pwm_config_t;

struct gpiote_int_data_t {
	uint32_t rtcTicks;
	uint32_t pinVal;
};

/** Pulse Wide Modulation class
 *
 * To turn on/off the power, as well as all intermediate stages, for example with dimming, the PWM class is used.
 * 
 * TODO: The PWM class concerns dimming in general. It should be called "Dimmer", not just "PWM".
 */
class PWM {
public:
	//! Gets a static singleton (no dynamic memory allocation) of the PWM class
	static PWM& getInstance() {
		static PWM instance;
		return instance;
	}

	static const uint8_t _maxValue = 100;

	/**
	 * Initialize the PWM settings.
	 * config can be safely deleted after calling this function.
	 */
	uint32_t init(const pwm_config_t& config);

	//! De-Initialize the PWM instance, i.e. free allocated resources
	uint32_t deinit();

	/** Start PWM.
	 *
	 * @param[in] onZeroCrossing       Set to true to make it start at the first zero crossing.
	 */
	uint32_t start(bool onZeroCrossing);

	/**
	 * Set the value of a specific channel.
	 *
	 * Each tick, the value will go towards target value by increasing or decreasing the actual value with 'speed'.
	 *
	 * @param[in] channel    Channel to set.
	 * @param[in] value      Target value to set the channel to (0-100).
	 * @param[in] speed      Speed at which to go to target value (1-100).
	 */
	void setValue(uint8_t channel, uint8_t value, uint8_t speed);

	//! Get current value of a specific channel.
	uint8_t getValue(uint8_t channel);

	/**
	 * Function to be called on the end of the PWM period.
	 *
	 * Decoupled from interrupt.
	 */
	void onPeriodEnd();

	//! Function to be called on a zero crossing interrupt.
	void onZeroCrossingInterrupt();

	//! Internal use! Called when started from a zero crossing.
	void _zeroCrossingStart();

	//! Interrupt handler: internal function, implementation specific.
	void _handleInterrupt();

	//! Helper function to get the gpiote event, given the index.
	static nrf_gpiote_events_t getGpioteEvent(uint8_t index);

	uint32_t _prevRtcTicks = 0;
	uint32_t _lastAdcZeroCrossInterruptRtcTicks = 0;

//	const static uint32_t _pinZeroCross = 32 + 9;
	const static uint32_t _pinZeroCross = 11;
	const static uint32_t _gpioteZeroCross = CS_PWM_GPIOTE_CHANNEL_START + CS_PWM_GPIOTE_CHANNEL_COUNT;

	void _handleGpioteInterrupt(gpiote_int_data_t* data);

	void _handleAdcInterrupt(uint32_t rtcTicks);

private:
	//! Private PWM constructor
	PWM();

	//! Private PWM copy constructor
	PWM(PWM const&);

	//! Private PWM copy assignment definition
	void operator=(PWM const &);

	//! Config of the PWM.
	pwm_config_t _config;

	//! Duty cycle values of the channels in percentage.
	uint8_t _values[CS_PWM_MAX_CHANNELS] = {0};

	//! Flag to indicate that the init function has been successfully performed
	bool _initialized;

	//! Flag to indicate that the start functions has been successfully performed
	bool _started;

	//! Flag to indicate whether to wait for a zero crossing to start.
	bool _startOnZeroCrossing;

	//! Init a channel
	uint32_t initChannel(uint8_t index, pwm_channel_config_t& config);

	/**
	 * Start the PWM timer, and mark PWM as started.
	 *
	 * Can be called from zero crossing interrupt.
	 */
	void start();

	/**
	 * Check if the current value with the target value.
	 *
	 * If not equal, set the value.
	 */
	void updateValues();

	/**
	 * Actually set the value in the peripheral.
	 */
	void setValue(uint8_t channel, uint8_t newValue);



	//! Max value of channel, in ticks. Set at init
	uint32_t _maxTickVal;

	//! Max value of channel, in ticks. Adjusted to sync with zero crossings.
	uint32_t _adjustedMaxTickVal;

	/**
	 * Target duty cycle values of the channels in percentage.
	 */
	uint8_t _targetValues[CS_PWM_MAX_CHANNELS] = {0};

	/**
	 * Step size to move actual value towards target value.
	 */
	uint8_t _stepSize[CS_PWM_MAX_CHANNELS] = {0};

	/**
	 * Only update values every so many PWM periods.
	 */
	static const uint8_t numPeriodsBeforeValueUpdate = 3;

	/**
	 * Current number of periods to wait before next value update.
	 */
	uint8_t _updateValuesCountdown = 0;

	//! Duty cycle values of the channels in ticks.
	uint32_t _tickValues[CS_PWM_MAX_CHANNELS] = {0};

	//! PPI channels to be used to trigger GPIOTE tasks from timer compare events. Turning the switch on.
	nrf_ppi_channel_t _ppiChannelsOn[CS_PWM_MAX_CHANNELS];

	//! PPI channels to be used to trigger GPIOTE tasks from timer compare events. Turning the switch off.
	nrf_ppi_channel_t _ppiChannelsOff[CS_PWM_MAX_CHANNELS];

	//! PPI channel to be used for duty cycle transitions.
	nrf_ppi_channel_t _ppiTransitionChannel;

	//! GPIOTE init states cache
	nrf_gpiote_outinit_t _gpioteInitStatesOn[CS_PWM_MAX_CHANNELS];
	nrf_gpiote_outinit_t _gpioteInitStatesOff[CS_PWM_MAX_CHANNELS];

	//! Returns whether a channel is currently dimming (value > 0 and < max).
	bool _isPwmEnabled[CS_PWM_MAX_CHANNELS];

	//! Counter to keep up the number of zero crossing callbacks.
	uint32_t _zeroCrossingCounter;

	//! Moving average amount of timer ticks deviation compared to when the zero crossing callback was called.
	int64_t _zeroCrossTicksDeviationAvg;

	//! Integral of the tick deviations.
	int64_t _zeroCrossDeviationIntegral;

	//! Config gpiote
	void gpioteConfig(uint8_t channel, bool initOn);
	//! Unconfig gpiote
	void gpioteUnconfig(uint8_t channel);
	//! Enable gpiote
	void gpioteEnable(uint8_t channel);
	//! Disable gpiote
	void gpioteDisable(uint8_t channel);
	//! Force gpiote state
	void gpioteForce(uint8_t channel, bool initOn);
	//! Turn on with gpio
	void gpioOn(uint8_t channel);
	//! Turn off with gpio
	void gpioOff(uint8_t channel);
	//! Write CC of timer
	void writeCC(uint8_t channelIdx, uint32_t ticks);
	//! Read CC of timer
	uint32_t readCC(uint8_t channelIdx);


	//! Enables the timer interrupt, to change the pwm value.
	void enableInterrupt();

	//! Helper function to get the timer channel, given the index.
	nrf_timer_cc_channel_t getTimerChannel(uint8_t index);
	//! Helper function to get the gpiote task out, given the index.
	nrf_gpiote_tasks_t getGpioteTaskOut(uint8_t index);
	//! Helper function to get the gpiote task out, given the index.
	nrf_gpiote_tasks_t getGpioteTaskSet(uint8_t index);
	//! Helper function to get the gpiote task out, given the index.
	nrf_gpiote_tasks_t getGpioteTaskClear(uint8_t index);
	//! Helper function to get the ppi channel, given the index.
	nrf_ppi_channel_t getPpiChannel(uint8_t index);
	//! Helper function to get the ppi group, given the index.
	nrf_ppi_channel_group_t getPpiGroup(uint8_t index);
	//! Helper function to get the ppi enable task, given the group index.
	nrf_ppi_task_t getPpiTaskEnable(uint8_t index);
	//! Helper function to get the ppi disable task, given the group index.
	nrf_ppi_task_t getPpiTaskDisable(uint8_t index);
};
