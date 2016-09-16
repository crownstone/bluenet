/**
 * Author: Dominik Egger
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 9 Mar., 2016
 * License: LGPLv3+, Apache, and/or MIT, your choice
 */

#include <drivers/cs_PWM.h>
#include <util/cs_BleError.h>
#include <drivers/cs_Serial.h>
#include <cfg/cs_Strings.h>
#include <cfg/cs_Config.h>
#include <cfg/cs_Boards.h>

/**********************************************************************************************************************
 * MACROS used in pulse width modulation class
 *********************************************************************************************************************/

// For increasing the level selectively for PWM, locally increase the log level via this macro.
#define SERIAL_INCREASE_LOG_LEVEL 0

// The PWM_ENABLE macro can be used to disable functionality at precompilation time.
// #define PWM_ENABLE

/**********************************************************************************************************************
 * Pulse Width Modulation class
 *********************************************************************************************************************/

PWM::PWM() :
		pwmTimer(NULL), 
		_pwmInstance(NULL), 
		_initialized(false),
		_inverted(false) {

#if PWM_ENABLE==1

	//! See APP_PWM_INSTANCE(name, num)
#if (NORDIC_SDK_VERSION >= 11) //! Not sure if 11 is the first version
	pwmTimer = new nrf_drv_timer_t();
	pwmTimer->p_reg = PWM_TIMER; // Or use CONCAT_2(NRF_TIMER, PWM_TIMER_ID)
	pwmTimer->instance_id = PWM_INSTANCE_INDEX; // Or use CONCAT_3(TIMER, PWM_TIMER_ID, _INSTANCE_INDEX)
	pwmTimer->cc_channel_count = NRF_TIMER_CC_CHANNEL_COUNT(PWM_TIMER_ID);

	_pwmInstance = new (app_pwm_t) {
		.p_cb = &_controlBlock,
		.p_timer = pwmTimer,
	};
#else
	pwmTimer = new nrf_drv_timer_t();
	pwmTimer->p_reg = PWM_TIMER;
	pwmTimer->irq = PWM_IRQn;
	pwmTimer->instance_id = PWM_INSTANCE_INDEX;

	_pwmInstance = new (app_pwm_t) {
		.p_cb = &_controlBlock,
		.p_timer = pwmTimer,
	};
#endif

#endif

#ifdef SWITCH_INVERSED
	_inverted = true;
#else
	_inverted = false;
#endif
}

//! Flag indicating PWM status
static volatile bool ready_flag;

//! PWM callback function
void pwm_ready_callback(uint32_t pwm_id) {   
	ready_flag = true;
}

void PWM::setValue(uint8_t channel, uint32_t value) {
	if (!_initialized) {
		LOGe(FMT_NOT_INITIALIZED, "PWM");
		return;
	}
	if (value > 100) {
		value = 100;
	}

	log(SERIAL_DEBUG + SERIAL_INCREASE_LOG_LEVEL, "Set PWM channel %d to %d", channel, value);

	if (_inverted) {
		log(SERIAL_DEBUG + SERIAL_INCREASE_LOG_LEVEL, "Switch inversed");
	} else {
		log(SERIAL_DEBUG + SERIAL_INCREASE_LOG_LEVEL, "Switch not inversed");
	}

	while (app_pwm_channel_duty_set(_pwmInstance, channel, value) == NRF_ERROR_BUSY) {
	};
}

uint32_t PWM::getValue(uint8_t channel) {
	if (!_initialized) {
		LOGe(FMT_NOT_INITIALIZED, "PWM");
		return 0;
	}
	return app_pwm_channel_duty_get(_pwmInstance, channel);
}

void PWM::switchOff() {
	if (!_initialized) {
		LOGe(FMT_NOT_INITIALIZED, "PWM");
		return;
	}
	for (uint32_t i = 0; i < _pwmCfg.num_of_channels; ++i) {
		setValue(i, 0);
	}
}

uint32_t PWM::init(app_pwm_config_t & config) {

#if PWM_ENABLE==1
	_pwmCfg = config;

	logLN(SERIAL_DEBUG + SERIAL_INCREASE_LOG_LEVEL, FMT_INIT, "PWM");

	BLE_CALL(app_pwm_init, (_pwmInstance, &_pwmCfg, pwm_ready_callback));
	app_pwm_enable(_pwmInstance);

	_initialized = true;

	return 0;
#endif

	return ERR_PWM_NOT_ENABLED;

}

uint32_t PWM::deinit() {

#if PWM_ENABLE==1

	log(SERIAL_DEBUG + SERIAL_INCREASE_LOG_LEVEL, "DeInit PWM");

	app_pwm_disable(_pwmInstance);
	BLE_CALL(app_pwm_uninit, (_pwmInstance));
	_initialized = false;
	return 0;
#endif

	return ERR_PWM_NOT_ENABLED;
}

app_pwm_config_t & PWM::config1Ch(uint32_t period, uint32_t pin) {
	static app_pwm_config_t cfg = APP_PWM_DEFAULT_CONFIG_1CH(period, pin);

	if (_inverted) {
		cfg.pin_polarity[0] = APP_PWM_POLARITY_ACTIVE_LOW;
	} else { 
		cfg.pin_polarity[0] = APP_PWM_POLARITY_ACTIVE_HIGH;
	}

	return cfg;
}

app_pwm_config_t & PWM::config2Ch(uint32_t period, uint32_t pin1, uint32_t pin2) {
	static app_pwm_config_t cfg = APP_PWM_DEFAULT_CONFIG_2CH(period, pin1, pin2);

	if (_inverted) {
		cfg.pin_polarity[0] = APP_PWM_POLARITY_ACTIVE_LOW;
		cfg.pin_polarity[1] = APP_PWM_POLARITY_ACTIVE_LOW;
	} else {
		cfg.pin_polarity[0] = APP_PWM_POLARITY_ACTIVE_HIGH;
		cfg.pin_polarity[1] = APP_PWM_POLARITY_ACTIVE_HIGH;
	}

	return cfg;
}

