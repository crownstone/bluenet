/**
 * Author: Anne van Rossum
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 6 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

#include <cstdint>

#include <ble/cs_Nordic.h>

extern "C" {
#include "nrf_drv_timer.h"
#include "app_pwm.h"
}

//#define PWM1

///* Pulse Wide Modulation struct
// */
//struct pwm_config_t {
//    uint8_t         num_channels;
//    uint8_t         gpio_pin[3];
//    uint8_t         ppi_channel[6];
//    uint8_t         gpiote_channel[3];
//    uint8_t         mode;
//
//    // default values
//    pwm_config_t() :
//    	num_channels   (3),
//		gpio_pin       {8,9,10},
//		ppi_channel    {0,1,2,3,4,5},
//		gpiote_channel {2,3,0},
//		mode           (PWM_MODE_122)
//    {}
//};

//void pwm_ready_callback(uint32_t pwm_id);    // PWM callback function

/* Pulse Wide Modulation class
 *
 * To turn on/off the power, as well as all intermediate stages, for example with dimming, the PWM class is used.
 */
class PWM {

//	int32_t ppiEnableChannel(uint32_t ch_num, volatile uint32_t *event_ptr, volatile uint32_t *task_ptr);
public:
	// Gets a static singleton (no dynamic memory allocation) of the PWM clss
	static PWM& getInstance() {
		static PWM instance;
		return instance;
	}

//	static volatile bool _pwmReady;

//	static void pwmReadyCallback(uint32_t pwmId);
//	static void pwmReadyCallback(uint32_t pwmId) {
//		_pwmReady = true;
//	};

	// Initialize the pulse wide modulation settings
	uint32_t init(app_pwm_config_t config);

	static app_pwm_config_t config1Ch(uint32_t period, uint32_t pin);
	static app_pwm_config_t config2Ch(uint32_t period, uint32_t pin1, uint32_t pin2);

	uint32_t deinit();

	// Set the value of a specific channel
	void setValue(uint8_t channel, uint32_t value);

	// Switch off all channels
	// Also works when not initialized (useful for emergencies)
	void switchOff();

	uint32_t getValue(uint8_t channel);
	
	//TODO: make the following private

//	// number of channels
//	uint8_t _numChannels;
//	// TODO -oDE: can we make this type consistent with _pwmValue? and uint8_t should be enough?
//	uint16_t _maxValue;
//	// TODO -oDE: can we make this type consistent with _pwmValue? and uint8_t should be enough?
//	uint16_t _nextValue[PWM_MAX_CHANNELS];
//	uint8_t _gpioteChannel[PWM_MAX_CHANNELS];
//	uint8_t _gpioPin[PWM_MAX_CHANNELS];
//	bool _running[PWM_MAX_CHANNELS];
//	bool _initialized;

private:
	// Private PWM constructor
	PWM() {};

	// Private PWM copy constructor
	PWM(PWM const&);
	// Private PWM copy assignment definition
	void operator=(PWM const &);

	app_pwm_config_t _pwmCfg;
//	app_pwm_t* _pwmInstance;
	uint32_t _callbackArray[APP_PWM_CB_SIZE];
	const nrf_drv_timer_t pwmTimer = NRF_DRV_TIMER_INSTANCE(2);
	app_pwm_t _pwmInstance = {
		.p_cb = &_callbackArray,
		.p_timer = &pwmTimer,
	};
};
