# ADC
-------------------------

The ADC samples one or multiple pins using the SAADC peripheral.


## Normal operation

In normal operation the ADC works roughly like this:

1. The SAADC is given a 2 buffers.
- The SAADC fills the 1st buffer with sampled values, every time a hardware timer ticks.
- The SAADC triggers an interrupt that the buffer is filled, and continues with the 2nd buffer.
- The 1st buffer is processed by the user.
- The 1st buffer is given to the SAADC.
- Etc.

A detailed diagram can be found [here](uml/adc/normal-operations.svg)


## Delays

However, sometimes the CPU is busy. This can lead to the processing being delayed, or the interrupt being delayed. If that happens, the SAADC doesn't get a new buffer before the previous buffer has been filled. But since the hardware timer keeps on ticking, the SAADC keeps sampling, with no place to leave the samples. They get discarded and you end up with a gap in measurements and in case of multiple pins, values on the wrong place.

To safeguard against these issues, there is a timeout timer that is stopped every time a buffer has been processed, and started when a new buffer is being filled.

If a timeout occurs, the ADC is stopped and restarted. It will wait for every buffer to be processed before restarting.

See the detailed diagrams for [delayed processing](uml/adc/delayed-processing.svg) and [delayed interrupt](uml/adc/delayed-interrupt.svg).


## Configuration change

The config of the ADC can be reconfigured on the fly. The ADC will wait until a buffer is filled, then it will stop the ADC, reconfigure the SAADC, restart, and finally let the buffer be processed. Again, it will only actually start when the buffer has been processed. See the [diagram](uml/adc/config-change.svg)
