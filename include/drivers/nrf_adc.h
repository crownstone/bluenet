/**
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 6 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#ifndef __NRF_ADC__H__
#define __NRF_ADC__H__

#include <stdint.h>

#include <drivers/nrf_rtc.h>

#include <common/buffer.h>

class ADC {
	public:
		ADC(): _buffer_size(100) {}
		virtual ~ADC() {}

		uint32_t nrf_adc_init(uint8_t pin);
		uint32_t nrf_adc_config(uint8_t pin);
		void nrf_adc_start();
		void nrf_adc_stop();

		buffer_t<uint16_t>* getBuffer();
	protected:

	private:
		int _buffer_size;
};

#endif
