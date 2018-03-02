/*
 * Author: Anne van Rossum
 * Copyright: Crownstone B.V.
 * Date: Mar 1, 2018
 * License: LGPLv3+, MIT, Apache
 */
#pragma once

#include <cstdlib>

//#include "common/cs_Types.h"
//#include "drivers/cs_Serial.h"
//#include <cfg/cs_Strings.h>

#include <cfg/cs_Config.h>
#include <nrf.h>  

/** Interleaved Buffer implementation
 *
 */
class InterleavedBuffer {
private:

	nrf_saadc_value_t* _buf[CS_ADC_NUM_BUFFERS];

public:
	static InterleavedBuffer& getInstance() {
		static InterleavedBuffer instance;
		return instance;
	}

	nrf_saadc_value_t* getBuffer(uint8_t index) {
		return _buf[index];
	}

	void setBuffer(uint8_t index, nrf_saadc_value_t* ptr) {
		_buf[index] = ptr;
	}

	cs_adc_buffer_id_t getIndex(nrf_saadc_value_t *buffer) {
		for (cs_adc_buffer_id_t i = 0; i < CS_ADC_NUM_BUFFERS; ++i) {
			if (buffer == _buf[i]) 
				return i;
		}
		return CS_ADC_NUM_BUFFERS;
	}

	bool exists(nrf_saadc_value_t *buffer) {
		return (getIndex(buffer) != CS_ADC_NUM_BUFFERS);
	}
};
