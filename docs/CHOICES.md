# Firmware choices

This document will explain some of the choices made for the architecture and implementation of the firmware.


### ADC

The ADC samples both current and voltage measurements. We use the SAADC peripheral of the bluetooth chip to do so. It comes with the option to sample multiple pins directly to a buffer RAM with easyDMA, this means there is no CPU time involved. Only when the buffer is full, an interrupt will be fired so that you can read out the buffer. In the meanwhile, the SAADC will continue to fill a second buffer. Once you're done with the first buffer, you give it back to the SAADC to be filled again, and you'll get the second buffer to read later on.

Unfortunately, the SAADC has a [bug](https://devzone.nordicsemi.com/question/97728/saadc-scan-mode-sample-order-is-not-always-consistent/): sometimes the values of the two pins swap places in the buffer. This seems to happen mostly when the softdevice is busy (for example when connecting to the crownstone). There is a workaround for old chips where the multiple pin sampling didn't work: it simply triggers an interrupt every sample, and the input pin gets switched there. This results in less stable sampling timing, but at least we know which value belongs to which pin.

#### Zero crossing detection

The SAADC also has the option to fire a limit interrupt when the sampled value goes below the lower threshold or above the upper threshold.
We start with the upper threshold at zero, and the lower threshold at the minimum. At the limit interrupt the lower threshold is set at zero, and the upper at the maximum, this way we get an interrupt at zero crossings.

Since we don't know the exact zero, we only use the upward zero crossings, so that at least the time between the zero crossings is stable.

Since the ADC is not constantly sampling, we sometimes get an interrupt before a zero crossing. In order to overcome this problem, we check if the previous upward zero crossing was about 20ms ago.


### Power sampling

The buffers from the ADC are processed by the power sampling class. Since the zero of the voltage and current are not stable, these are first calculated by taking the averare of a single period (20ms). This value is then smoothed by an exponential moving average.

The real power is calculated by multiplying voltage with current at every time step, and taking the average of this over a period. This is a different number than the apparent power, which is V<sub>rms</sub> * I<sub>rms</sub> (see [wikipedia](https://en.wikipedia.org/wiki/AC_power)). Again, the calculated power is smoothed by calculating the exponential moving average. This is required, because the measured signal is noisy due to interference with the radio.


### Dimming

In order to support most LED lights, and to elongate the lifetime of incandescent lamps, the dimmer is implemented as trailing edge PWM. This means the IGBTs should turn on at the zero crossing of the voltage (and turn off somewhere half-way the cycle).

Since the zero crossing interrupts are not too accurate (and some are even skipped), we can't simply turn the IGBTs on at the zero crossing interrupt. Instead, the PWM is running at 100Hz and synchronized with the 50Hz of the mains.

Again, because the zero crossings are not accurate enough, we can't just "reset" the PWM at the interrupt; this led to visible flickering, since a small adjustment has a big influence on the power output.

So instead, the PWM is synchronized by slightly adjusting the period. The feedback is given by storing the PWM timer ticks at the zero crossing interrupt. When the PWM is perfectly synchronized, the timer should be at 0 ticks at the zero crossing.
This means that if the tick count is higher than 0, the period is decreased. While if the tick count is below zero (or actually a number above half the period number of ticks), the period is increased. This basically is a control system, that has to be properly tweaked, or else it can get instable.


## Soft fuses

There are several things we can monitor for protection: current and temperature.
The IGBTs have a PTC or NTC to measure the temperature, and the chip has an on board temperature sensor. Temperature rises relatively slowly and should be the last line of defense. Current can be measured quickly (in order of ms), but interference can cause false readings, leading to a slower response time to avoid false positives.

When the chip temperature is too high, the most likely cause is that a large current went through the relay for an extended period, while the Crownstone was unable to get rid of the generated heat.

When the IGBT temperature is too high, the most likely cause is that a large current went thought the IGBTs for some time, while no over current was measured. In this case, the dimmer will be turned off.

### Relay

The relay (and dimmer) will be turned off when:

- The current exceeds 16A.
- The chip temperature is too high.

A flag will be set, and the relay, nor the dimmer will be allowed to be turned on again.

### Dimmer

The dimmer will be turned off when:

- The current exceeds about 0.5A.
- The IGBT temperature is too high.

There is a slight chance, however, that the dimmer is broken and unable to be switched off. This is why the relay is turned on as well (and remains on). This outranks the relay being turned off (or remain off) by the other events. This makes sure the overheating will be minimized, as the relay path generated the least heat.
