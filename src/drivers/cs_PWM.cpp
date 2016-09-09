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

//#define PRINT_PWM_VERBOSE

PWM::PWM() :
		pwmTimer(NULL), _pwmInstance(NULL), _initialized(false) {

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

}

static volatile bool ready_flag;            // A flag indicating PWM status.

void pwm_ready_callback(uint32_t pwm_id) {   // PWM callback function
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

#ifdef PRINT_PWM_VERBOSE
	LOGd("Set PWM channel %d to %d", channel, value);
#endif
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

uint32_t PWM::init(app_pwm_config_t config) {

#if PWM_ENABLE==1
	_pwmCfg = config;

#ifdef PRINT_PWM_VERBOSE
	LOGd(FMT_INIT, "PWM");
#endif

	BLE_CALL(app_pwm_init, (_pwmInstance, &_pwmCfg, pwm_ready_callback));
	app_pwm_enable(_pwmInstance);

	_initialized = true;

	return 0;
#endif

	return ERR_PWM_NOT_ENABLED;

}

uint32_t PWM::deinit() {

#if PWM_ENABLE==1

#ifdef PRINT_PWM_VERBOSE
	LOGd("DeInit PWM");
#endif

	app_pwm_disable(_pwmInstance);
	BLE_CALL(app_pwm_uninit, (_pwmInstance));
	_initialized = false;
	return 0;
#endif

	return ERR_PWM_NOT_ENABLED;
}

app_pwm_config_t PWM::config1Ch(uint32_t period, uint32_t pin) {
	app_pwm_config_t cfg = APP_PWM_DEFAULT_CONFIG_1CH(period, pin);

#ifdef SWITCH_INVERSED
	cfg.pin_polarity[0] = APP_PWM_POLARITY_ACTIVE_HIGH;
#else
	cfg.pin_polarity[0] = APP_PWM_POLARITY_ACTIVE_LOW;
#endif

	return cfg;
}

app_pwm_config_t PWM::config2Ch(uint32_t period, uint32_t pin1, uint32_t pin2) {
	app_pwm_config_t cfg = APP_PWM_DEFAULT_CONFIG_2CH(period, pin1, pin2);

#ifdef SWITCH_INVERSED
	cfg.pin_polarity[0] = APP_PWM_POLARITY_ACTIVE_HIGH;
	cfg.pin_polarity[1] = APP_PWM_POLARITY_ACTIVE_HIGH;
#else
	cfg.pin_polarity[0] = APP_PWM_POLARITY_ACTIVE_LOW;
	cfg.pin_polarity[1] = APP_PWM_POLARITY_ACTIVE_LOW;
#endif

	return cfg;
}

