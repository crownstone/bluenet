# ADC
-------------------------

The ADC samples one or multiple pins using the SAADC peripheral.


## Normal operation

In normal operation the ADC works roughly like this:

1. The SAADC is given a 2 buffers.
- The SAADC fills the first buffer with sampled values, every time a hardware timer ticks.
- The SAADC triggers an interrupt that the buffer is filled, and continues with the second buffer.
- The first buffer is processed by the user.
- The first buffer is given to the SAADC.
- Etc.

A detailed diagram can be found [here](uml/adc/normal-operation.svg).

## Offset in samples

There can be an offset in the samples in the buffer. The voltage and current values are interleaved. An odd offset makes voltage values current values  and the other way around. This is **not** offset due to temperature. (For this the ADC needs to be [recalibrated](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.nrf52832.ps.v1.1%2Fsaadc.html&cp=2_1_0_36_5&anchor=saadc_easydma) at times).

The engineers at Nordic are quite aware of this issue. However, the proposed solution in practice has not been sufficient.

* https://devzone.nordicsemi.com/f/nordic-q-a/20291/offset-in-saadc-samples-with-easy-dma-and-ble
* https://devzone.nordicsemi.com/f/nordic-q-a/16885/saadc-scan-mode-sample-order-is-not-always-consistent/134614#134614
* https://devzone.nordicsemi.com/f/nordic-q-a/36443/glitches-in-adc-sampling

The "solution" is to actually use the PPI to trigger the START task on an END event. Say channel `1`. We do this in [drivers/cs_ADC.cpp](https://github.com/crownstone/bluenet/blob/master/source/src/drivers/cs_ADC.cpp#L203):

    _ppiChannelStart = getPpiChannel(CS_ADC_PPI_CHANNEL_START+1);
    nrf_ppi_channel_endpoint_setup(
		  _ppiChannelStart,
		  nrf_saadc_event_address_get(NRF_SAADC_EVENT_END),
		  nrf_saadc_task_address_get(NRF_SAADC_TASK_START)

What this solves is that a SAMPLE event is not accidentally sent to the ADC before a TASK_START after an END event. If there is a single sample event missing at the beginning of the buffer all voltage/current values are indeed exchanged.

What is suggested [by Bart](https://devzone.nordicsemi.com/f/nordic-q-a/20291/offset-in-saadc-samples-with-easy-dma-and-ble) is that `RESULT.PTR` is set too late. This pointer is double-buffered. It can be updated and prepared for the next `START` event immediately after a `STARTED` event has been generated. Namely, after `START` the contents of this register is copied into an internal SAADC register.

1. Let's assume `RESULT.PTR` is not set after processing the buffer (a copy action or worse which can take considerable amount of time), but immediately after a `STARTED` event. 
2. This means that for a period of a buffer (a couple of milliseconds), there has been no way to deliver a `STARTED` event.
3. Or the function (running on a low interrupt level) that checks for `STARTED` didn't get CPU time yet. There is a [while loop](https://github.com/crownstone/bluenet/blob/master/source/src/drivers/cs_ADC.cpp#L488) in addBufferToSampleQueue.

Things to check:

1. Do we indeed do no processing of the buffer before we set `RESULT.PTR`? We really shouldn't... Rather indicate a buffer as dirty / non-processed and disregard in software
2. Is there no way to pick up the `STARTED` event in an interrupt service routine and set `RESULT.PTR` from there?

## Delays

However, sometimes the CPU is busy. This can lead to the processing being delayed, or the interrupt being delayed. If that happens, the SAADC doesn't get a new buffer before the previous buffer has been filled. But since the hardware timer keeps on ticking, the SAADC keeps sampling, with no place to leave the samples. They get discarded and you end up with a gap in measurements and in case of multiple pins, values on the wrong place.

To safeguard against these issues, there is a timeout timer that is stopped every time a buffer has been processed, and started when a new buffer is being filled.

If a timeout occurs, the ADC is stopped and restarted. It will wait for every buffer to be processed before restarting.

See the detailed diagrams for [delayed processing](uml/adc/delayed-processing.svg) and [delayed interrupt](uml/adc/delayed-interrupt.svg).


## Configuration change

The config of the ADC can be reconfigured on the fly. The ADC will wait until a buffer is filled, then it will stop the ADC, reconfigure the SAADC, restart, and finally let the buffer be processed. Again, it will only actually start when the buffer has been processed. See the [diagram](uml/adc/config-change.svg).
