/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Jan 31, 2017
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <ble/cs_Nordic.h>
#include <drivers/cs_COMP.h>
#include <drivers/cs_RTC.h>
#include <logging/cs_Logger.h>
#include <protocol/cs_ErrorCodes.h>
#include <util/cs_BleError.h>

extern "C" void comp_callback(nrf_comp_event_t event);

COMP::COMP() {}

void COMP::init(uint8_t ainPin, float thresholdDown, float thresholdUp, comp_event_cb_t callback) {
	// thresholdDown has to be lower than thresholdUp, else the comp shows weird behaviour (main thread hangs)
	if (thresholdDown >= thresholdUp) {
		APP_ERROR_HANDLER(NRF_ERROR_INVALID_PARAM);
	}

	LOGd("init pin=%u down=%i up=%i",
		 ainPin,
		 NRFX_VOLTAGE_THRESHOLD_TO_INT(thresholdDown, 3.3),
		 NRFX_VOLTAGE_THRESHOLD_TO_INT(thresholdUp, 3.3));
#ifdef NRF52_PAN_12
	applyWorkarounds();
#endif

	// TODO: get VDD from board config.
	nrf_comp_th_t threshold = {
			.th_down = NRFX_VOLTAGE_THRESHOLD_TO_INT(thresholdDown, 3.3),
			.th_up   = NRFX_VOLTAGE_THRESHOLD_TO_INT(thresholdUp, 3.3)};

	_callback = callback;

	nrfx_comp_config_t config;
	config.reference  = NRF_COMP_REF_VDD;
	config.main_mode  = NRF_COMP_MAIN_MODE_SE;  // Single ended, not differential
	config.threshold  = threshold;
	config.speed_mode = NRF_COMP_SP_MODE_Low;  // Delay of 0.6us
	config.hyst       = NRF_COMP_HYST_NoHyst;  // Not used in single ended mode, use thresholds instead.
#if defined(NRF52840_XXAA) || defined(NRF52840_XXAA_ENGA)
	// None of the NRF52 devices have a functional ISOURCE. It has been removed for the NRF52840. Not a real fix no.
#else
	config.isource = NRF_COMP_ISOURCE_Off;  // Should be off in our case and due to PAN 84
#endif
	config.interrupt_priority = APP_IRQ_PRIORITY_LOW;

	switch (ainPin) {
		case 0:
			config.input = NRF_COMP_INPUT_0;  // AIN0, gpio 2
			break;
		case 1:
			config.input = NRF_COMP_INPUT_1;  // AIN1, gpio 3
			break;
		case 2:
			config.input = NRF_COMP_INPUT_2;  // AIN2, gpio 4
			break;
		case 3:
			config.input = NRF_COMP_INPUT_3;  // AIN3, gpio 5
			break;
		case 4:
			config.input = NRF_COMP_INPUT_4;  // AIN4
			break;
		case 5:
			config.input = NRF_COMP_INPUT_5;  // AIN5
			break;
		case 6:
			config.input = NRF_COMP_INPUT_6;  // AIN6
			break;
		case 7:
			config.input = NRF_COMP_INPUT_7;  // AIN7
			break;
		default: APP_ERROR_HANDLER(NRF_ERROR_INVALID_PARAM);
	}

	ret_code_t nrfCode = nrfx_comp_init(&config, comp_callback);
	switch (nrfCode) {
		case NRFX_SUCCESS:
			// * @retval NRFX_SUCCESS             If initialization was successful.
			break;
		case NRFX_ERROR_INVALID_STATE:
			// * @retval NRFX_ERROR_INVALID_STATE If the driver has already been initialized.
		case NRFX_ERROR_BUSY:
			// * @retval NRFX_ERROR_BUSY          If the LPCOMP peripheral is already in use.
			// *                                  This is possible only if @ref nrfx_prs module
			// *                                  is enabled.
		default:
			// Crash.
			APP_ERROR_HANDLER(nrfCode);
	}
}

void COMP::start(CompEvent_t event) {
	LOGd("start");
	uint32_t evtMask;
	switch (event) {
		case COMP_EVENT_BOTH: evtMask = NRF_COMP_EVENT_UP | NRF_COMP_EVENT_DOWN; break;
		case COMP_EVENT_UP: evtMask = NRF_COMP_EVENT_UP; break;
		case COMP_EVENT_DOWN: evtMask = NRF_COMP_EVENT_DOWN; break;
		case COMP_EVENT_CROSS: evtMask = NRF_COMP_EVENT_CROSS; break;
		default: return;
	}
	nrfx_comp_start(evtMask, 0);
}

bool COMP::sample() {
	uint32_t sample = nrfx_comp_sample();
	return static_cast<bool>(sample);
}

void compEventCallback(void* p_event_data, uint16_t event_size) {
	nrf_comp_event_t event = *reinterpret_cast<nrf_comp_event_t*>(p_event_data);
	COMP::getInstance().handleEventDecoupled(event);
}

void COMP::handleEventDecoupled(nrf_comp_event_t event) {
	if (_callback == nullptr) {
		return;
	}
	switch (event) {
		case NRF_COMP_EVENT_READY: break;
		case NRF_COMP_EVENT_DOWN: _callback(COMP_EVENT_DOWN); break;
		case NRF_COMP_EVENT_UP: _callback(COMP_EVENT_UP); break;
		case NRF_COMP_EVENT_CROSS: _callback(COMP_EVENT_CROSS); break;
		default: break;
	}
}

void COMP::handleEvent(nrf_comp_event_t event) {
	// Decouple callback from the interrupt handler, and put it on app scheduler instead
	if (_callback != nullptr) {
		uint32_t errorCode = app_sched_event_put(&event, sizeof(event), compEventCallback);
		APP_ERROR_CHECK(errorCode);
	}
}

extern "C" void comp_callback(nrf_comp_event_t event) {
	COMP::getInstance().handleEvent(event);
}
