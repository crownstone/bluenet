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

PWM::PWM() :
		_initialized(false)
		{
#if PWM_ENABLE==1

#endif
}

uint32_t PWM::init(pwm_config_t & config) {
//#if PWM_ENABLE==1
	LOGi(FMT_INIT, "PWM");
	_config = config;

	uint32_t err_code;

	_timer = new nrf_drv_timer_t();
	_timer->p_reg = CS_PWM_TIMER;
	_timer->instance_id = CS_PWM_INSTANCE_INDEX;
	_timer->cc_channel_count = NRF_TIMER_CC_CHANNEL_COUNT(CS_PWM_TIMER_ID);

	nrf_drv_timer_config_t timerConfig;
	timerConfig.frequency          = NRF_TIMER_FREQ_1MHz;
	timerConfig.mode               = (nrf_timer_mode_t)TIMER_MODE_MODE_Timer;
	timerConfig.bit_width          = (nrf_timer_bit_width_t)TIMER_BITMODE_BITMODE_32Bit;
	timerConfig.interrupt_priority = CS_PWM_TIMER_PRIORITY;
	timerConfig.p_context          = NULL;
	err_code = nrf_drv_timer_init(_timer, &timerConfig, staticTimerHandler);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_ppi_init();
	if (err_code != MODULE_ALREADY_INITIALIZED) {
		APP_ERROR_CHECK(err_code);
	}

	if (!nrf_drv_gpiote_is_init()) {
		err_code = nrf_drv_gpiote_init();
		APP_ERROR_CHECK(err_code);
	}

	for (uint8_t i=0; i<_config.channelCount; ++i) {
		LOGd("configure channel %u at pin %u", i, _config.channels[i].pin);

		// Config gpiote
		nrf_drv_gpiote_out_config_t gpioteOutConfig;
		if (_config.channels[i].inverted) {
			gpioteOutConfig.init_state = NRF_GPIOTE_INITIAL_VALUE_HIGH;
			nrf_gpio_pin_set(_config.channels[i].pin);
		}
		else {
			gpioteOutConfig.init_state = NRF_GPIOTE_INITIAL_VALUE_LOW;
			nrf_gpio_pin_clear(_config.channels[i].pin);
		}
		gpioteOutConfig.task_pin   = true;
		gpioteOutConfig.action     = NRF_GPIOTE_POLARITY_TOGGLE;
		err_code = nrf_drv_gpiote_out_init((nrf_drv_gpiote_pin_t)_config.channels[i].pin, &gpioteOutConfig);
		APP_ERROR_CHECK(err_code);

		// Config ppi
		err_code = nrf_drv_ppi_channel_alloc(&_ppiChannels[i*2]);
		APP_ERROR_CHECK(err_code);
		err_code = nrf_drv_ppi_channel_alloc(&_ppiChannels[i*2+1]);
		APP_ERROR_CHECK(err_code);

		err_code = nrf_drv_ppi_channel_assign(
				_ppiChannels[i*2],
//				nrf_drv_timer_compare_event_address_get(_timer, NRF_TIMER_CC_CHANNEL0 + i),
				nrf_drv_timer_compare_event_address_get(_timer, i),
				nrf_drv_gpiote_out_task_addr_get(_config.channels[i].pin));
		APP_ERROR_CHECK(err_code);

		err_code = nrf_drv_ppi_channel_assign(
				_ppiChannels[i*2+1],
//				nrf_drv_timer_compare_event_address_get(_timer, NRF_TIMER_CC_CHANNEL5),
				nrf_drv_timer_compare_event_address_get(_timer, 5),
				nrf_drv_gpiote_out_task_addr_get(_config.channels[i].pin));
		APP_ERROR_CHECK(err_code);
	}

	uint32_t ticks = nrf_drv_timer_us_to_ticks(_timer, _config.period_us);
	LOGd("period=%uus ticks=%u", _config.period_us, ticks);
	nrf_drv_timer_clear(_timer);
//	nrf_drv_timer_extended_compare(_timer, (nrf_timer_cc_channel_t) NRF_TIMER_CC_CHANNEL5, ticks, NRF_TIMER_SHORT_COMPARE5_CLEAR_MASK, false);
    nrf_drv_timer_extended_compare(_timer, (nrf_timer_cc_channel_t) NRF_TIMER_CC_CHANNEL5, ticks, NRF_TIMER_SHORT_COMPARE5_CLEAR_MASK, true);
    nrf_drv_timer_compare_int_enable(_timer, NRF_TIMER_CC_CHANNEL5);
//	nrf_drv_timer_compare_int_disable(_timer, NRF_TIMER_CC_CHANNEL5);

    sd_nvic_EnableIRQ(CS_PWM_IRQn);

    // Enable
    for (uint8_t i=0; i<_config.channelCount; ++i) {
    	nrf_drv_gpiote_out_task_force(_config.channels[i].pin, _config.channels[i].inverted ? NRF_GPIOTE_INITIAL_VALUE_HIGH : NRF_GPIOTE_INITIAL_VALUE_LOW);
    	nrf_drv_gpiote_out_task_enable(_config.channels[i].pin);
    	nrf_drv_ppi_channel_enable(_ppiChannels[i*2]);
    	nrf_drv_ppi_channel_enable(_ppiChannels[i*2+1]);
    }
    //    nrf_drv_timer_clear(_timer);
    nrf_drv_timer_enable(_timer);

    // Set at 25%
    for (uint8_t i=0; i<_config.channelCount; ++i) {
    	uint32_t value = ticks / 4;
    	nrf_drv_timer_compare(_timer, (nrf_timer_cc_channel_t)(NRF_TIMER_CC_CHANNEL0 + i), value, false);
    }

    _initialized = true;
    return ERR_SUCCESS;
//#endif

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
	LOGd("Set PWM channel %d to %d", channel, value);

}

uint16_t PWM::getValue(uint8_t channel) {
	if (!_initialized) {
		LOGe(FMT_NOT_INITIALIZED, "PWM");
		return 0;
	}

	LOGe("Invalid channel");
	return 0;
}

// Interrupt handler
void PWM::staticTimerHandler(nrf_timer_event_t event_type, void* ptr) {
//	if (event_type == NRF_TIMER_EVENT_COMPARE5) {
		write("tick\r\n");
//	}
}
