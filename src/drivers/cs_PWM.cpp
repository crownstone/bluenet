/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 9 Mar., 2016
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#include "drivers/cs_PWM.h"
#include "util/cs_BleError.h"
#include "drivers/cs_Serial.h"
#include "cfg/cs_Strings.h"
#include "cfg/cs_Config.h"
#include "protocol/cs_ErrorCodes.h"

#include <nrf.h>
#include <app_util_platform.h>

/**********************************************************************************************************************
 * Pulse Width Modulation class
 *********************************************************************************************************************/

//nrf_pwm_values_individual_t PWM::_nrfValues = { .channel_0 = 0, .channel_1 = 0, .channel_2 = 0, .channel_3 = 0 };
//nrf_drv_pwm_t PWM::_nrfPwm0 = { .p_registers = NRF_PWM0, .drv_inst_idx = 0 };
//nrf_pwm_sequence_t PWM::_nrfSeq = { .values.p_individual = &(PWM::_nrfValues), .length = NRF_PWM_VALUES_LENGTH(_nrfValues), .repeats = 0, .end_delay = 0 };
nrf_drv_pwm_t PWM::_nrfPwm = {};
nrf_pwm_values_individual_t PWM::_nrfValues = {};
nrf_pwm_sequence_t PWM::_nrfSeq = {};

PWM::PWM() :
		_initialized(false)
		{

#if PWM_ENABLE==1
//	_nrfPwm0 = NRF_DRV_PWM_INSTANCE(0);
	_nrfPwm.p_registers = NRF_PWM0;
	_nrfPwm.drv_inst_idx = 0;
	_nrfSeq.values.p_individual = &_nrfValues;
	_nrfSeq.length = NRF_PWM_VALUES_LENGTH(_nrfValues);
	_nrfSeq.repeats = 0;
	_nrfSeq.end_delay = 0;
#endif
}

uint32_t PWM::init(pwm_config_t & config) {
#if PWM_ENABLE==1
	_config = config;

	LOGi(FMT_INIT, "PWM");

	/* is this true?
	// The pwm peripheral starts the period off, then goes on at the end of the period.
	// Since we want to start the period on, we have to invert the polarity and thus set the duty cycle at: max - value
	 */

	// PWM frequency = clock frequency / top_value (for NRF_PWM_MODE_UP)
	// PWM period = clock period * top_value (for NRF_PWM_MODE_UP)
	// Fully on is top_value

	// NRF_PWM_MODE_UP_AND_DOWN means the counter first goes up, then down. See PS for a good explanation.

	nrf_drv_pwm_config_t nrfConfig;
	for (uint8_t i=0; i<NRF_PWM_CHANNEL_COUNT; ++i) {
		nrfConfig.output_pins[i] = NRF_DRV_PWM_PIN_NOT_USED;
	}
	for (uint8_t i=0; i<_config.channelCount; ++i) {
		nrfConfig.output_pins[i] = _config.channels[i].pin;
		if (_config.channels[i].inverted == true) {
			nrfConfig.output_pins[i] |= NRF_DRV_PWM_PIN_INVERTED;
		}
	}
	nrfConfig.irq_priority = APP_IRQ_PRIORITY_LOW;
	nrfConfig.base_clock  = NRF_PWM_CLK_1MHz;
	nrfConfig.count_mode  = NRF_PWM_MODE_UP;
	nrfConfig.top_value   = 10000; // 100Hz = 1MHz / 10000
	nrfConfig.load_mode   = NRF_PWM_LOAD_INDIVIDUAL; // Each channel has different values
	nrfConfig.step_mode   = NRF_PWM_STEP_AUTO;

	uint32_t err_code = nrf_drv_pwm_init(&_nrfPwm, &nrfConfig, NULL);
	APP_ERROR_CHECK(err_code);

	_nrfValues.channel_0 = 0;
	_nrfValues.channel_1 = 0;
	_nrfValues.channel_2 = 0;
	_nrfValues.channel_3 = 0;

	uint32_t flags = NRF_DRV_PWM_FLAG_LOOP | NRF_DRV_PWM_FLAG_NO_EVT_FINISHED;
//	uint32_t flags = NRF_DRV_PWM_FLAG_LOOP;
	nrf_drv_pwm_simple_playback(&_nrfPwm, &_nrfSeq, 1, flags);
	LOGd("pwm initialized!");

	_initialized = true;
	return ERR_SUCCESS;
#endif

	return ERR_PWM_NOT_ENABLED;
}

uint32_t PWM::deinit() {

#if PWM_ENABLE==1

	LOGi("DeInit PWM");

	_initialized = false;
	return ERR_SUCCESS;
#endif

	return ERR_PWM_NOT_ENABLED;
}

void PWM::restart() {
	if (!_initialized) {
		LOGe(FMT_NOT_INITIALIZED, "PWM");
		return;
	}
	// Tried, doesn't work
//	uint32_t flags = NRF_DRV_PWM_FLAG_LOOP | NRF_DRV_PWM_FLAG_NO_EVT_FINISHED;
//	nrf_drv_pwm_simple_playback(&_nrfPwm, &_nrfSeq, 1, flags);
}


void PWM::setValue(uint8_t channel, uint16_t value) {
	if (!_initialized) {
		LOGe(FMT_NOT_INITIALIZED, "PWM");
		return;
	}
	if (value > 100) {
		value = 100;
	}
	value = value*100; // Should be top_value at value 100

	LOGd("Set PWM channel %d to %d", channel, value);

	switch (channel) {
	case 0:
		_nrfValues.channel_0 = value;
		break;
	case 1:
		_nrfValues.channel_1 = value;
		break;
	case 2:
		_nrfValues.channel_2 = value;
		break;
	case 3:
		_nrfValues.channel_3 = value;
		break;
	default:
		return;
	}

	// No need for this
//	uint32_t flags = NRF_DRV_PWM_FLAG_LOOP | NRF_DRV_PWM_FLAG_NO_EVT_FINISHED;
//	nrf_drv_pwm_simple_playback(&_nrfPwm, &_nrfSeq, 1, flags);
}

uint16_t PWM::getValue(uint8_t channel) {
	if (!_initialized) {
		LOGe(FMT_NOT_INITIALIZED, "PWM");
		return 0;
	}

	switch (channel) {
	case 0:
		return _nrfValues.channel_0;
	case 1:
		return _nrfValues.channel_1;
	case 2:
		return _nrfValues.channel_2;
	case 3:
		return _nrfValues.channel_3;
	}
	LOGe("Invalid channel");
	return 0;
}
