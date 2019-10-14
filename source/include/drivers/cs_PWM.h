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
#include "events/cs_EventDispatcher.h"

#define ERR_PWM_NOT_ENABLED 1

/**
 * Number of zero crossings to calculate the slope of the error.
 * /!\ Currently hard coded to be 7 /!\
 */
#define DIMMER_NUM_CROSSINGS_PER_SLOPE_ESTIMATE 7

/**
 * Number of slope estimates until the timer max ticks are adjusted to synchronize with the grid frequency.
 */
#define DIMMER_NUM_SLOPE_ESTIMATES_FOR_FREQUENCY_SYNC 7

/**
 * Number of zero crossings until the timer max ticks is adjusted to synchronize the timer start with the grid zero crossing.
 */
#define DIMMER_NUM_CROSSINGS_FOR_START_SYNC 7

/**
 * Number of timer start synchronizations until synchronizing frequency again.
 */
#define DIMMER_NUM_START_SYNCS_BETWEEN_FREQ_SYNC 1000


typedef struct {
	uint16_t pin;
	bool inverted;
} pwm_channel_config_t;

typedef struct {
	uint32_t period_us;
	uint8_t channelCount;
	pwm_channel_config_t channels[CS_PWM_MAX_CHANNELS];
} pwm_config_t;

/** Pulse Wide Modulation class
 *
 * To turn on/off the power, as well as all intermediate stages, for example with dimming, the PWM class is used.
 */
class PWM : EventListener {
public:
	//! Gets a static singleton (no dynamic memory allocation) of the PWM class
	static PWM& getInstance() {
		static PWM instance;
		return instance;
	}

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
	 * TODO: currently calling this twice within 10ms, will lead to errors!
	 */
	void setValue(uint8_t channel, uint16_t value);

	//! Get current value of a specific channel.
	uint16_t getValue(uint8_t channel);

	//! Function to be called on a zero crossing.
	void onZeroCrossing();

	//! Internal use! Called when started from a zero crossing.
	void _zeroCrossingStart();

	//! Interrupt handler: internal function, implementation specific.
	void _handleInterrupt();

	//! Event handler: to measure the offset between the detected zero crossing timer value and the true zero timer value
	void handleEvent(event_t & event);

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
	uint32_t _values[CS_PWM_MAX_CHANNELS];

	//! Flag to indicate that the init function has been successfully performed
	bool _initialized;

	//! Flag to indicate that the start functions has been successfully performed
	bool _started;

	//! Flag to indicate whether to wait for a zero crossing to start.
	bool _startOnZeroCrossing;

	//! If a zero crossing interrupt is missed, there's no point of trying to compensate via the event.
	bool _skipZeroCrossingEvent;

	//! Init a channel
	uint32_t initChannel(uint8_t index, pwm_channel_config_t& config);

	//! Actually start the PWM
	void start();

	// -----------------------------------
	// ----- Implementation specific -----
	// -----------------------------------

	//! RTC's timer ticks to measure the time between interrupts
	uint32_t _rtcTimerVal;

	//! Max value of channel, in timer ticks. Set at init.
	uint32_t _maxTickVal;

	//! Max value of channel, in timer ticks. Adjusted to sync frequency with zero crossings.
	uint32_t _freqSyncedMaxTickVal;

	//! Max value of channel, in timer ticks. Adjusted to sync with zero crossings.
	uint32_t _adjustedMaxTickVal;

	/**
	 * New duty cycle values of the channels in percentage.
	 * Used in case the value couldn't be adjusted yet at the moment of calling setValue.
	 */
	uint32_t _nextValues[CS_PWM_MAX_CHANNELS];

	//! Duty cycle values of the channels in ticks.
	uint32_t _tickValues[CS_PWM_MAX_CHANNELS];

	//! PPI channels to be used to communicate from Timer to GPIOTE.
	nrf_ppi_channel_t _ppiChannels[2*CS_PWM_MAX_CHANNELS];

	//! PPI channels to be used for duty cycle transitions.
	nrf_ppi_channel_t _ppiTransitionChannels[2];

	//! PPI group, used to enable/disable ppi channels.
	nrf_ppi_channel_group_t _ppiGroup;

	//! GPIOTE init states cache
	nrf_gpiote_outinit_t _gpioteInitStatesOn[CS_PWM_MAX_CHANNELS];
	nrf_gpiote_outinit_t _gpioteInitStatesOff[CS_PWM_MAX_CHANNELS];

	//! Returns whether a channel is currently dimming (value > 0 and < max).
	bool _isPwmEnabled[CS_PWM_MAX_CHANNELS];

	//! Counter to keep up the number of zero crossing callbacks.
	uint32_t _zeroCrossingCounter = 0;

	//! Array of tick counts at the moment of a zero crossing interrupt.
	int32_t _offsets[MAX(DIMMER_NUM_CROSSINGS_PER_SLOPE_ESTIMATE, DIMMER_NUM_CROSSINGS_FOR_START_SYNC)];

	//! Array of calculated slopes between 1 point and all other points.
	int32_t _offsetSlopes[DIMMER_NUM_CROSSINGS_PER_SLOPE_ESTIMATE-1];
	//! Array of median of calculated slopes.
	int32_t _offsetSlopes2[DIMMER_NUM_CROSSINGS_PER_SLOPE_ESTIMATE];
	//! Array of median of median of calculated slopes.
	int32_t _offsetSlopes3[DIMMER_NUM_SLOPE_ESTIMATES_FOR_FREQUENCY_SYNC];

	//! Integral of the tick offset.
	int32_t _zeroCrossOffsetIntegral;

	//! Current PWM timer tick value; needed for zero crossing slope compensation
	uint32_t _currTicks = 0;

	//! Whether currently synchronizing frequency (else synchronizing timer start).
	bool _syncFrequency;

	//! Number of times we performed synchronization since last change of _syncFrequency..
	uint32_t _numSyncs;

	//! Checks if there was a missed interrupt
	bool isInterruptMissed();

	//! Sets pin on
	void turnOn(uint8_t channel);

	//! Sets pin off
	void turnOff(uint8_t channel);

	//! Enables pwm for given channel (to be used when value is > 0 and < max).
	void enablePwm(uint8_t channel);

	//! Disables pwm for given channel (to be used when value is 0 or max).
	void disablePwm(uint8_t channel);

//	//! Enables pwm for given channel and turns it on or off.
//	void enablePwm(uint8_t channel, bool on);
//
	//! Disables pwm for given channel and turns it on or off.
	void disablePwm(uint8_t channel, bool on);

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
	//! Wrap around a value, so that it ends up in the range [-max/2, max/2]
	void wrapAround(int32_t& val, int32_t max);

	//! Convert time from microseconds to ticks based on the timer's clock frequency
	int32_t convert_us_to_ticks(int32_t time_us);

	//! Get the PWM driver's synced frequency value to be as close as possible to the mains supply frequency value
	void performCoarseFrequencySyncing();

	//! Make minor corrections (i.e. predict next zero crossing values) once every few zero crossings
	void performPWMTimerCorrections();

	//! Function to be called when the offset (in μs) of the previous zero crossing was calculated.
	void onZeroCrossingTimeOffset(int32_t offset);

	//! Enables the timer interrupt, to change the pwm value.
	void enableInterrupt();

	//! Helper function to get the timer channel, given the index.
	nrf_timer_cc_channel_t getTimerChannel(uint8_t index);
	//! Helper function to get the gpiote task out, given the index.
	nrf_gpiote_tasks_t getGpioteTaskOut(uint8_t index);
	//! Helper function to get the ppi channel, given the index.
	nrf_ppi_channel_t getPpiChannel(uint8_t index);
	//! Helper function to get the ppi group, given the index.
	nrf_ppi_channel_group_t getPpiGroup(uint8_t index);
	//! Helper function to get the ppi enable task, given the group index.
	nrf_ppi_task_t getPpiTaskEnable(uint8_t index);
	//! Helper function to get the ppi disable task, given the group index.
	nrf_ppi_task_t getPpiTaskDisable(uint8_t index);
};

#if DIMMER_NUM_CROSSINGS_PER_SLOPE_ESTIMATE == 4
#define slopeMedian(arr) opt_med3(arr)
#elif DIMMER_NUM_CROSSINGS_PER_SLOPE_ESTIMATE == 6
#define slopeMedian(arr) opt_med5(arr)
#elif DIMMER_NUM_CROSSINGS_PER_SLOPE_ESTIMATE == 7
#define slopeMedian(arr) opt_med6(arr)
#elif DIMMER_NUM_CROSSINGS_PER_SLOPE_ESTIMATE == 8
#define slopeMedian(arr) opt_med7(arr)
#elif DIMMER_NUM_CROSSINGS_PER_SLOPE_ESTIMATE == 10
#define slopeMedian(arr) opt_med9(arr)
#elif DIMMER_NUM_CROSSINGS_PER_SLOPE_ESTIMATE == 26
#define slopeMedian(arr) opt_med25(arr)
#else
#error "Invalid value for DIMMER_NUM_CROSSINGS_PER_SLOPE_ESTIMATE"
#endif

#if DIMMER_NUM_SLOPE_ESTIMATES_FOR_FREQUENCY_SYNC == 3
#define medianSlopeMedian(arr) opt_med3(arr)
#elif DIMMER_NUM_SLOPE_ESTIMATES_FOR_FREQUENCY_SYNC == 5
#define medianSlopeMedian(arr) opt_med5(arr)
#elif DIMMER_NUM_SLOPE_ESTIMATES_FOR_FREQUENCY_SYNC == 6
#define medianSlopeMedian(arr) opt_med6(arr)
#elif DIMMER_NUM_SLOPE_ESTIMATES_FOR_FREQUENCY_SYNC == 7
#define medianSlopeMedian(arr) opt_med7(arr)
#elif DIMMER_NUM_SLOPE_ESTIMATES_FOR_FREQUENCY_SYNC == 9
#define medianSlopeMedian(arr) opt_med9(arr)
#elif DIMMER_NUM_SLOPE_ESTIMATES_FOR_FREQUENCY_SYNC == 25
#define medianSlopeMedian(arr) opt_med25(arr)
#else
#error "Invalid value for DIMMER_NUM_SLOPE_ESTIMATES_FOR_FREQUENCY_SYNC"
#endif

#if DIMMER_NUM_CROSSINGS_FOR_START_SYNC == 3
#define errorMedian(arr) opt_med3(arr)
#elif DIMMER_NUM_CROSSINGS_FOR_START_SYNC == 5
#define errorMedian(arr) opt_med5(arr)
#elif DIMMER_NUM_CROSSINGS_FOR_START_SYNC == 6
#define errorMedian(arr) opt_med6(arr)
#elif DIMMER_NUM_CROSSINGS_FOR_START_SYNC == 7
#define errorMedian(arr) opt_med7(arr)
#elif DIMMER_NUM_CROSSINGS_FOR_START_SYNC == 9
#define errorMedian(arr) opt_med9(arr)
#elif DIMMER_NUM_CROSSINGS_UNTIL_ADJUSTMENT == 25
#define errorMedian(arr) opt_med25(arr)
#else
#error "Invalid value for DIMMER_NUM_CROSSINGS_FOR_START_SYNC"
#endif
