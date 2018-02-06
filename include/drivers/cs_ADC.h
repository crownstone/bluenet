/*
 * Author: Crownstone Team
 * Copyright: Crownstone
 * Date: 6 Nov., 2014
 * Triple-license: LGPLv3+, Apache License, and/or MIT
 */
#pragma once

#include <ble/cs_Nordic.h>
#include <cfg/cs_Config.h>
#include <structs/buffer/cs_CircularBuffer.h>
#include <structs/buffer/cs_StackBuffer.h>
#include <structs/buffer/cs_DifferentialBuffer.h>

//! Numeric reference to a pin
typedef uint8_t cs_adc_pin_id_t;

//! Numeric reference to a channel
typedef uint8_t cs_adc_channel_id_t;

//! Numeric reference to a buffer
typedef uint8_t cs_adc_buffer_id_t;

//! Pin count (number)
typedef uint8_t cs_adc_pin_count_t;

//! Buffer count (number)
typedef uint8_t cs_adc_buffer_count_t;

//! Buffer size (number)
typedef uint16_t cs_adc_buffer_size_t;

//! Error codes (number)
typedef uint32_t cs_adc_error_t;

/**
 * The typedef adc_done_cb_t is a function pointer to (1) a buffer with elements of type nrf_saadc_value_t (currently 
 * defined by Nordic to be int16_t), (2) the size of the buffer in the sense of the number of these elements, and 
 * (3) an index to the buffer. This function pointer can be given as argument to ADC::setDoneCallback to set a callback
 * on an ADC event. Currently, ADC::setDoneCallback is called from cs_PowerSampling.cpp.
 */
typedef void (*adc_done_cb_t) (nrf_saadc_value_t* buf, cs_adc_buffer_size_t size, cs_adc_buffer_id_t bufNum);

typedef void (*adc_zero_crossing_cb_t) ();

/** Struct storing data for ADC callback.
 *
 * The struct adc_done_cb_data_t stores (1) a callback pointer, see adc_done_cb_t, (2) the arguments for this pointer,
 * see also adc_done_cb_t (a pointer to the buffer with data from the ADC conversion, the buffer size, and a unique
 * number for the buffer).
 */
struct adc_done_cb_data_t {
	//! Callback function
	adc_done_cb_t callback;
	//! Buffer as argument for ADC callback 
	nrf_saadc_value_t* buffer;
	//! Buffer size as argument for ADC callback 
	cs_adc_buffer_size_t bufSize;
	//! Buffer index as argument for ADC callback
	//! TODO: remove this field
	cs_adc_buffer_id_t bufNum;
};

enum adc_gain_t {
	CS_ADC_GAIN4   = NRF_SAADC_GAIN4,   // gain is 4/1, maps [0, 0.15] to [0, 0.6]
	CS_ADC_GAIN2   = NRF_SAADC_GAIN2,   // gain is 2/1, maps [0, 0.3]  to [0, 0.6]
	CS_ADC_GAIN1   = NRF_SAADC_GAIN1,   // gain is 1/1, maps [0, 0.6]  to [0, 0.6]
	CS_ADC_GAIN1_2 = NRF_SAADC_GAIN1_2, // gain is 1/2, maps [0, 1.2]  to [0, 0.6]
	CS_ADC_GAIN1_3 = NRF_SAADC_GAIN1_3, // gain is 1/3, maps [0, 1.8]  to [0, 0.6]
	CS_ADC_GAIN1_4 = NRF_SAADC_GAIN1_4, // gain is 1/4, maps [0, 2.4]  to [0, 0.6]
	CS_ADC_GAIN1_5 = NRF_SAADC_GAIN1_5, // gain is 1/5, maps [0, 3.0]  to [0, 0.6]
	CS_ADC_GAIN1_6 = NRF_SAADC_GAIN1_6, // gain is 1/6, maps [0, 3.6]  to [0, 0.6]
};

#define CS_ADC_REF_PIN_NOT_AVAILABLE 255
#define CS_ADC_PIN_VDD 100

/** Struct to configure an adc channel.
 *
 * pin:               The AIN pin to sample.
 * rangeMilliVolt:    The range in mV of the pin.
 * referencePin:      The AIN pin to be subtracted from the measured voltage.
 */
struct __attribute__((packed)) adc_channel_config_t {
	cs_adc_pin_id_t pin;
	uint32_t rangeMilliVolt;
	cs_adc_pin_id_t referencePin;
};

/** Struct to configure the adc.
 *
 * channelCount:      The amount of channels to sample.
 * channels:          The channel configs.
 * samplingPeriodUs:  The sampling period in μs (each period, all channels are sampled once).
 */
struct __attribute__((packed)) adc_config_t {
	cs_adc_channel_id_t channelCount;
	adc_channel_config_t channels[CS_ADC_MAX_PINS];
	uint32_t samplingPeriodUs;
};

/** Analog-Digital conversion.
 *
 * The ADC is a hardware peripheral that can be initialized for a particular pin. Subsequently, the analog signal
 * is converted into a digital value. The resolution of the conversion is limited (default 10 bits). The conversion
 * is used to get the current curve.
 *
 * There is only one ADC hardware peripheral, hence this class conforms to the singleton design pattern. 
 *
 * The constructor does NOT have arguments you might expect, such as the number of buffers to use, or the size of the 
 * buffers. These are all set through preprocessor macros. The ADC class requires access to the following macros
 * (see below).
 *
 * The timer used to get new samples on regular time intervals. The timer should be able to run on at least 2kHz to
 * be able to get the fine-grained current and voltage data we want.
 *
 *   - CS_ADC_TIMER                    The timer peripheral to use for the ADC peripheral.
 *   - CS_ADC_INSTANCE_INDEX           The instance id of the timer. TODO: What is this?
 *   - CS_ADC_TIMER_ID                 TODO: What is this?
 *
 * The buffers to be used internally. To have these buffers in the form of macros means that we can check at compile
 * time if they are small enough with respect to the nRF52 memory limitations.
 *
 *   - CS_ADC_NUM_BUFFERS              The number of ADC buffers to use. 
 *   - CS_ADC_BUF_SIZE                 The size of the buffer (first time used in init()).
 *
 * The pins:
 *
 *   - CS_ADC_MAX_PINS                 The maximum number of pins available for ADC.
 *
 * The sequence of events is as follows:
 *
 * 1. The C interrupt service routine, saadc_callback(nrf_drv_saadc_evt_t const * p_event) can be found in the 
 * corresponding implementation file. This function uses the singleton feature to call update on an event from the 
 * ADC peripheral, in particular, NRF_DRV_SAADC_EVT_DONE, with a buffer as argument.
 *
 * 2. The update() routine puts an internal C function, adc_done, with _doneCallback as argument in the event queue
 * of the scheduler. This calls the callback function set previously in setDoneCallback() by a particular caller, 
 * e.g. the cs_PowerSampling. In this particular case it ends up in PowerSampling::powerSampleAdcDone that after 
 * processing the data calls releaseBuffer(bufNum).
 *
 * 3. The releaseBuffer function clears the _doneCallbackData structure and queues a new buffer. Note that all 
 * processing has taken place by now.
 
 * Note. In the ADC it is assumed that the member object is not updated (_doneCallbackData.bufName == bufNum) in the
 * mean while. In other words, it should not be possible to get a second interrupt before releaseBuffer has been 
 * called. Scheduling a new ADC task is only done at the end of the releaseBuffer function.
 */
class ADC {

public:
	//! Use static variant of singleton, no dynamic memory allocation
	static ADC& getInstance() {
		static ADC instance;
		return instance;
	}

	/** Initialize ADC.
	 *
	 * @param[in] config               Config struct.
	 * @return                         Error code (0 for success).
	 */
	cs_adc_error_t init(const adc_config_t& config);

	/** Start the ADC sampling
	 */
	void start();

	/** Stop the ADC sampling
	 */
	void stop();

	/** Release a buffer, so it can be used again by the ADC. 
	 *
	 * @param[in] buf                  Pointer to the buffer, as received in the done callback.
	 * @return                         Boolean indicating success (true) or failure (false).
	 */
	bool releaseBuffer(nrf_saadc_value_t* buf);

	/** Set the callback which is called when a buffer is filled.
	 *
	 * @param[in] callback             Function to be called when a buffer is filled with samples.
	 */
	void setDoneCallback(adc_done_cb_t callback);

	/** Set the callback which is called on a zero crossing interrupt.
	 *
	 * @param[in] callback             Function to be called on a zero crossing event. This function will run at interrupt level!
	 */
	void setZeroCrossingCallback(adc_zero_crossing_cb_t callback);

	/** Enable zero crossing detection on given channel, generating interrupts.
	 *
	 * @param[in] channel              The channel on which to check for zero crossings.
	 * @param[in] zeroVal              The value of zero.
	 */
	void enableZeroCrossingInterrupt(cs_adc_channel_id_t channel, int32_t zeroVal);

	/** Change channel config.
	 *  Currently this config is applied immediately.
	 *  TODO: Apply config after a buffer has been filled.
	 *
	 * @param[in] channel              Channel number.
	 * @param[in] config               Config struct.
	 * @return                         Error code (0 for success).
	 */
	cs_adc_error_t changeChannel(cs_adc_channel_id_t channel, adc_channel_config_t& config);


	/** Update this object with a buffer with values from the ADC conversion.
	 *
	 * This update() function is also called from the ADC interrupt service routine. In this case an entire buffer has
	 * been made available. This is done by having the ADC peripheral directly writing into a part of the FLASH using
	 * the Easy-DMA peripheral and inter-peripheral communication (called PPI).
	 * Note that this function is time-sensitive. The number of activities in it are limited to a minimal number. The 
	 * values are only copied and not processed further.
	 *
	 * @param[in] buf                  A buffer with size defined in the constructor.
	 */
	void _handleAdcDoneInterrupt(nrf_saadc_value_t* buf);

	/** Called when the sampled value is above upper limit, or below lower limit.
	 *
	 */
	void _handleAdcLimitInterrupt(nrf_saadc_limit_t type);

protected:


private:
	/** Constructor
	 */
	ADC();

	//! This class is singleton, deny implementation
	ADC(ADC const&);
	
	//! This class is singleton, deny implementation
	void operator=(ADC const &);

	//! Whether or not the config should be changed.
	bool _changeConfig;

	//! Configuration of this class
	adc_config_t _config;

	//! PPI channel to be used to communicate from Timer to ADC peripheral.
	nrf_ppi_channel_t _ppiChannel;

	//! Array of pointers to buffers.
	nrf_saadc_value_t* _bufferPointers[CS_ADC_NUM_BUFFERS];

	//! Number of buffers that are queued to be populated by adc.
	cs_adc_buffer_count_t _numBuffersQueued;

	//! Arguments to the callback function
	adc_done_cb_data_t _doneCallbackData;

	//! The zero crossing callback.
	adc_zero_crossing_cb_t _zeroCrossingCallback;

	//! The channel which is checked for zero crossings.
	cs_adc_channel_id_t _zeroCrossingChannel;

	//! Store the timestamp of the last upwards zero crossing.
	uint32_t _lastZeroCrossUpTime;

	//! Store the zero value used to detect zero crossings.
	int32_t _zeroValue;

	//! Function to initialize the adc channels.
	cs_adc_error_t initChannel(cs_adc_channel_id_t channel, adc_channel_config_t& config);

	//! Set the adc limit such that it triggers when going above zero
	void setLimitUp();

	//! Set the adc limit such that it triggers when going below zero
	void setLimitDown();

	//! Function that returns the adc pin number, given the AIN number
	nrf_saadc_input_t getAdcPin(cs_adc_pin_id_t pinNum);

	//! Function that puts a buffer in queue to be populated with adc values.
	void addBufferToSampleQueue(nrf_saadc_value_t* buf);

	//! Function to apply a new config. Should be called when no buffers are are queued, nor being processed.
	void applyConfig();

	//! Helper function to get the ppi channel, given the index.
	nrf_ppi_channel_t getPpiChannel(uint8_t index);
};
