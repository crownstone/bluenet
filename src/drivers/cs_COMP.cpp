/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 31, 2017
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include "drivers/cs_COMP.h"

#include <nrf.h>
#include "util/cs_BleError.h"
#include "drivers/cs_Serial.h"
#include "drivers/cs_RTC.h"

extern "C" void comp_callback(nrf_comp_event_t event);

//extern "C" void comp_work_around() { *(volatile uint32_t *)0x40013540 = (*(volatile uint32_t *)0x10000324 & 0x00001F00) >> 8; }

COMP::COMP() {

}

void COMP::init(uint8_t ainPin, float thresholdDown, float thresholdUp) {
	//! thresholdDown has to be lower than thresholdUp, else the comp shows weird behaviour (main thread hangs)

	// TODO: If memory is not initialized, this assert becomes false!
#ifdef ALLOW_ASSERTS
	assert(thresholdDown < thresholdUp, "thresholdDown should be lower than thresholdUp");
#else
	if (thresholdDown >= thresholdUp) {
		thresholdUp = thresholdDown + 100;
	}
#endif

	LOGd("init %d %d", VOLTAGE_THRESHOLD_TO_INT(thresholdDown, 3.3), VOLTAGE_THRESHOLD_TO_INT(thresholdUp, 3.3));
	_lastEventTimestamp = RTC::getCount();
#ifdef NRF52_PAN_12
	applyWorkarounds();
//	comp_work_around();
#endif

	nrf_comp_th_t threshold = {
			.th_down = VOLTAGE_THRESHOLD_TO_INT(thresholdDown, 3.3),
			.th_up = VOLTAGE_THRESHOLD_TO_INT(thresholdUp, 3.3)
	};
//	nrf_comp_th_t threshold;
//	threshold.th_down = 30;
//	threshold.th_up = 34;

	nrf_drv_comp_config_t config;
	config.reference = NRF_COMP_REF_VDD;
	config.main_mode = NRF_COMP_MAIN_MODE_SE; // Single ended, not differential
	config.threshold = threshold;
	config.speed_mode = NRF_COMP_SP_MODE_Low; // Delay of 0.6us
	config.hyst = NRF_COMP_HYST_NoHyst; // Not used in single ended mode
	config.isource = NRF_COMP_ISOURCE_Off; // Should be off in our case and due to PAN 84
	config.interrupt_priority = APP_IRQ_PRIORITY_LOW;

	assert(ainPin < 8, "Comp: wrong pin number");
	switch(ainPin) {
	case 0:
		config.input = NRF_COMP_INPUT_0; // AIN0, gpio 2
		break;
	case 1:
		config.input = NRF_COMP_INPUT_1; // AIN1, gpio 3
		break;
	case 2:
		config.input = NRF_COMP_INPUT_2; // AIN2, gpio 4
		break;
	case 3:
		config.input = NRF_COMP_INPUT_3; // AIN3, gpio 5
		break;
	case 4:
		config.input = NRF_COMP_INPUT_4; // AIN4
		break;
	case 5:
		config.input = NRF_COMP_INPUT_5; // AIN5
		break;
	case 6:
		config.input = NRF_COMP_INPUT_6; // AIN6
		break;
	case 7:
		config.input = NRF_COMP_INPUT_7; // AIN7
		break;
	}

	ret_code_t err_code = nrf_drv_comp_init(&config, comp_callback);
	switch(err_code) {
		case NRF_ERROR_INVALID_PARAM:
			LOGe("Comparator: invalid param");
			break;
		case NRF_ERROR_INVALID_STATE:
			LOGe("Comparator: invalid state");
			break;
		case NRF_ERROR_BUSY:
			LOGe("Comparator: is already initialized");
			break;
	}
	return; // for now skip check
	APP_ERROR_CHECK(err_code);
}

void COMP::start(CompEvent_t event) {
	LOGd("start");
	uint32_t evtMask;
	switch(event) {
	case COMP_EVENT_BOTH:
		evtMask = NRF_DRV_COMP_EVT_EN_UP_MASK | NRF_DRV_COMP_EVT_EN_DOWN_MASK;
		break;
	case COMP_EVENT_UP:
		evtMask = NRF_DRV_COMP_EVT_EN_UP_MASK;
		break;
	case COMP_EVENT_DOWN:
		evtMask = NRF_DRV_COMP_EVT_EN_DOWN_MASK;
		break;
	case COMP_EVENT_CROSS:
		evtMask = NRF_DRV_COMP_EVT_EN_CROSS_MASK;
		break;
	case COMP_EVENT_NONE:
	default:
		return;
	}
	nrf_drv_comp_start(evtMask, 0);
//	nrf_comp_int_enable(COMP_INTENSET_UP_Msk | COMP_INTENSET_DOWN_Msk);
//	nrf_comp_task_trigger(NRF_COMP_TASK_START);
}

uint32_t COMP::sample() {
	uint32_t sample = nrf_drv_comp_sample();
//	nrf_comp_task_trigger(NRF_COMP_TASK_SAMPLE);
//	uint32_t sample = nrf_comp_result_get();
	return sample;
}

void COMP::setEventCallback(comp_event_cb_t callback) {
	_eventCallbackData.callback = callback;
}

void compEventCallback(void * p_event_data, uint16_t event_size) {
	comp_event_cb_data_t* cbData = (comp_event_cb_data_t*)p_event_data;
	cbData->callback(cbData->event);
}

void COMP::handleEvent(nrf_comp_event_t & event) {
	switch (event) {
	case NRF_COMP_EVENT_READY:
		break;
	case NRF_COMP_EVENT_DOWN:
		_eventCallbackData.event = COMP_EVENT_DOWN;
//		LOGd("down");
		break;
	case NRF_COMP_EVENT_UP:
		_eventCallbackData.event = COMP_EVENT_UP;
//		LOGd("up");
		break;
	case NRF_COMP_EVENT_CROSS:
		_eventCallbackData.event = COMP_EVENT_CROSS;
//		LOGd("cross");
		break;
	default:
		_eventCallbackData.event = COMP_EVENT_NONE;
	}


	//! Decouple callback from the interrupt handler, and put it on app scheduler instead
	if (_eventCallbackData.callback != NULL) {
		uint32_t curTime = RTC::getCount();
		uint32_t timeDiff = RTC::difference(curTime, _lastEventTimestamp);
		if (timeDiff > RTC::msToTicks(100)) {
			uint32_t errorCode = app_sched_event_put(&_eventCallbackData, sizeof(_eventCallbackData), compEventCallback);
			APP_ERROR_CHECK(errorCode);
		}
		_lastEventTimestamp = RTC::getCount();
	}
}

extern "C" void comp_callback(nrf_comp_event_t nrf_comp_event) {
	COMP::getInstance().handleEvent(nrf_comp_event);
}
