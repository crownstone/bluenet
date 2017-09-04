# Firmware choices

This document will explain some of the choices made for the architecture and implementation of the firmware.


### Power sampling



### Dimming

In order to support most LED lights, and to elongate the lifetime of incandescent lamps, the dimmer is implemented as trailing edge PWM. This means the IGBTs should turn on at the zero crossing of the voltage (and turn off somewhere half-way the cycle).

Since the zero crossing interrupts are not too accurate (and some are even skipped), we can't simply turn the IGBTs on at the zero crossing interrupt. Instead, the PWM is running at 100Hz and synchronized with the 50Hz of the mains.

Again, because the zero crossings are not accurate enough, we can't just "reset" the PWM at the interrupt; this led to visible flickering, since a small adjustment has a big influence on the power output.

So instead, the PWM is synchronized by slightly adjusting the period. The feedback is given by storing the PWM timer ticks at the zero crossing interrupt. When the PWM is perfectly synchronized, the timer should be at 0 ticks at the zero crossing.
This means that if the tick count is higher than 0, the period is decreased. While if the tick count is below zero (or actually a number above half the period number of ticks), the period is increased. This basically is a control system, that has to be properly tweaked, or else it can get instable.


