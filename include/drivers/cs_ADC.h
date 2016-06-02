/*
 * Author: Bart van Vliet
 * Copyright: Distributed Organisms B.V. (DoBots)
 * Date: 6 Nov., 2014
 * License: LGPLv3+, Apache License, or MIT, your choice
 */
#pragma once

//#include <stdint.h>
//
#include "structs/buffer/cs_CircularBuffer.h"
#include "structs/buffer/cs_StackBuffer.h"
#include "structs/buffer/cs_DifferentialBuffer.h"
#include "cfg/cs_Config.h"

//#include "common/cs_Types.h"

/** Analog-Digital conversion.
 *
 * The ADC is a hardware peripheral that can be initialized for a particular pin. Subsequently, the analog signal
 * is converted into a digital value. The resolution of the conversion is limited (default 10 bits). The conversion
 * is used to get the current curve.
 */
class ADC {

public:
	//! Use static variant of singleton, no dynamic memory allocation
	static ADC& getInstance() {
		static ADC instance;
		return instance;
	}

	/** Initialize ADC.
	 * @param pins Array of pins to use as input (AIN<pin>). ADC will alternate between these pins.
	 * @param size Size of the array.
	 *
	 * The init function must called once before operating the AD converter.
	 * Call it after you start the SoftDevice.
	 */
	uint32_t init(uint8_t pins[], uint8_t numPins);

	/** Start the ADC sampling
	 */
	void start();

	/** Stop the ADC sampling
	 */
	void stop();

	/** Set the buffers for the sample values.
	 * To be used in case of burst sampling.
	 * For now, these are expected to be of equal size.
	 *
	 * @param buffer Buffer to use.
	 * @param pinNum Nth pin of the pin array supplied in init().
	 *
	 * @return true on success.
	 */
	bool setBuffers(StackBuffer<uint16_t>* buffer, uint8_t pinNum);

	/** Set the buffers for the sample values.
	 * These are used in case of continuous sampling.
	 *
	 * @param buffer Buffer to use.
	 * @param pinNum Nth pin of the pin array supplied in init().
	 *
	 * @return true on success.
	 */
	bool setBuffers(CircularBuffer<uint16_t>* buffer, uint8_t pinNum);

	/** Set the buffers for the timestamps.
	 * To be used in case of burst sampling.
	 * For now, these are expected to be of equal size as the sample value buffers.
	 *
	 * @param buffer Buffer to use.
	 * @param pinNum Nth pin of the pin array supplied in init().
	 *
	 * @return true on success.
	 */
	bool setTimestampBuffers(DifferentialBuffer<uint32_t>* buffer, uint8_t pinNum);

	//! Function to be called from interrupt, do not do much there!
	void update(uint32_t value);

private:
	/** Constructor
	 */
//	ADC(): _buffers(NULL), _timeBuffers(NULL), _circularBuffers(NULL) {}
	ADC() {}

	//! This class is singleton, deny implementation
	ADC(ADC const&);
	//! This class is singleton, deny implementation
	void operator=(ADC const &);

	uint8_t _pins[CS_ADC_MAX_PINS];
	uint8_t _numPins;
	uint8_t _lastPinNum;
//	uint16_t _sampleNum;
	StackBuffer<uint16_t>* _buffers[CS_ADC_MAX_PINS];
	DifferentialBuffer<uint32_t>* _timeBuffers[CS_ADC_MAX_PINS];
	CircularBuffer<uint16_t>* _circularBuffers[CS_ADC_MAX_PINS];

	//! Function to set the input pin, this can be done after each sample. Only used internally!
	uint32_t config(uint8_t pin);
};
