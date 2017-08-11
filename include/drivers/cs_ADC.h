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
typedef uint8_t pin_id_t;

//! Numeric reference to a channel
typedef uint8_t channel_id_t;

//! Numeric reference to a buffer
typedef uint8_t buffer_id_t;

//! Pin count (number)
typedef uint8_t pin_count_t;

//! Buffer count (number)
typedef uint8_t buffer_count_t;

//! Buffer size (number)
typedef uint16_t buffer_size_t;

//! Error codes (number)
typedef uint32_t cs_adc_error_t;

/**
 * The typedef adc_done_cb_t is a function pointer to (1) a buffer with elements of type nrf_saadc_value_t (currently 
 * defined by Nordic to be int16_t), (2) the size of the buffer in the sense of the number of these elements, and 
 * (3) an index to the buffer. This function pointer can be given as argument to ADC::setDoneCallback to set a callback
 * on an ADC event. Currently, ADC::setDoneCallback is called from cs_PowerSampling.cpp.
 */
typedef void (*adc_done_cb_t) (nrf_saadc_value_t* buf, buffer_size_t size, buffer_id_t bufNum);

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
	buffer_size_t bufSize;
	//! Buffer index as argument for ADC callback
	//! TODO: remove this field
	buffer_id_t bufNum;
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
	 * The init function must called once before operating the AD converter. Call it after you start the SoftDevice.
	 *
	 * TODO: Not all pins can be used for ADC conversion. Check if the proper pins are assessed.
	 * TODO: Send array by reference, waste of stack.
	 *
	 * @param[in] pins                 Array of pins to use as input (AIN<pin>). ADC will alternate between these pins.
	 * @param[in] size                 Size of the array.
	 * @return                         Error code (0 means success).
	 */
	cs_adc_error_t init(const pin_id_t pins[], const pin_count_t numPins);

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
	 * @param[in] callback             Function to be called on an ADC event (any event)!
	 */
	void setDoneCallback(adc_done_cb_t callback);

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
	void update(nrf_saadc_value_t* buf);

	void limit(nrf_saadc_limit_t type);

protected:

	/** Callback for when the internal timer is started.
	 *
	 * @param[in] event_type           The ADC is coupled to a timer which emits events.
	 * @param[in] ptr                  @TODO: What's this?
	 */
	static void staticTimerHandler(nrf_timer_event_t event_type, void* ptr);

private:
	/** Constructor
	 */
	ADC();

	//! This class is singleton, deny implementation
	ADC(ADC const&);
	
	//! This class is singleton, deny implementation
	void operator=(ADC const &);

	//! Pins that will be used for the ADC. 
	pin_id_t _pins[CS_ADC_MAX_PINS];

	//! Number of pins to be used for the ADC.
	pin_count_t _numPins;

	//! Pointer to the timer to be used for sampling.
	nrf_drv_timer_t* _timer;

	//! PPI channel to be used to communicate from Timer to ADC peripheral.
	nrf_ppi_channel_t _ppiChannel;

	//! Array of pointers to buffers.
	nrf_saadc_value_t* _bufferPointers[CS_ADC_NUM_BUFFERS];

	//! Number of buffers that are queued to be populated by adc.
	buffer_count_t _numBuffersQueued;

	//! Arguments to the callback function
	adc_done_cb_data_t _doneCallbackData;

	//! Function to set the input pin, this can be done after each sample. Only used internally!
	cs_adc_error_t configPin(const channel_id_t channel, const pin_id_t pin);

	//! Function that returns the adc pin number, given the AIN number
	nrf_saadc_input_t getAdcPin(pin_id_t pinNum);

	//! Function that puts a buffer in queue to be populated with adc values.
	void addBufferToSampleQueue(nrf_saadc_value_t* buf);
};
