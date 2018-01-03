# Firmware choices

This document will explain some of the choices made for the architecture and implementation of the firmware.


## ADC

The ADC samples both current and voltage measurements. We use the SAADC peripheral of the bluetooth chip to do so. It comes with the option to sample multiple pins directly to a buffer RAM with easyDMA, this means there is no CPU time involved. Only when the buffer is full, an interrupt will be fired so that you can read out the buffer. In the meanwhile, the SAADC will continue to fill a second buffer. Once you're done with the first buffer, you give it back to the SAADC to be filled again, and you'll get the second buffer to read later on.

Unfortunately, the SAADC has a [bug](https://devzone.nordicsemi.com/question/97728/saadc-scan-mode-sample-order-is-not-always-consistent/): sometimes the values of the two pins swap places in the buffer. This seems to happen mostly when the softdevice is busy (for example when connecting to the crownstone). There is a workaround for old chips where the multiple pin sampling didn't work: it simply triggers an interrupt every sample, and the input pin gets switched there. This results in less stable sampling timing, but at least we know which value belongs to which pin.

### Zero crossing detection

The SAADC also has the option to fire a limit interrupt when the sampled value goes below the lower threshold or above the upper threshold.
We start with the upper threshold at zero, and the lower threshold at the minimum. At the limit interrupt the lower threshold is set at zero, and the upper at the maximum, this way we get an interrupt at zero crossings.

Since we don't know the exact zero, we only use the upward zero crossings, so that at least the time between the zero crossings is stable.

Since the ADC is not constantly sampling, we sometimes get an interrupt before a zero crossing. In order to overcome this problem, we check if the previous upward zero crossing was about 20ms ago.


### Power sampling

The buffers from the ADC are processed by the power sampling class. Since the zero of the voltage and current are not stable, these are first calculated by taking the averare of a single period (20ms). This value is then smoothed by an exponential moving average.

The real power is calculated by multiplying voltage with current at every time step, and taking the average of this over a period. This is a different number than the apparent power, which is V<sub>rms</sub> * I<sub>rms</sub> (see [wikipedia](https://en.wikipedia.org/wiki/AC_power)). Again, the calculated power is smoothed by calculating the exponential moving average. This is required, because the measured signal is noisy due to interference with the radio.


## Dimming

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


## State propagation of Crownstones

### Problem

When using the mesh, a crownstone can advertise an old state of another crownstone, which can be advertised after the new state is advertised.

This leads to conflicting states, meaning the user can for example see the switch state toggling multiple times.

### Considerations

1. When entering sphere, phone needs to get state from all crownstones.
2. Phone shouldn't show old state when:
    1. When toggling switch.
    2. When toggling switch multiple times (can still go wrong with current implementation, depends on mesh delay).
3. Multiple user with phones which are out of sync (more than 1s).

### Proposal

Assumption: the crownstones clocks are synchronized (on the second). This should be the case when the clocks are regularly (daily?) set by the phone via the mesh.

The proposal is to add timestamps to [state items](PROTOCOL.md#state_mesh_item) on the mesh message, as well as timestamps to the [advertisement](PROTOCOL.md#scan_response_servicedata_packet).

The advertised timestamp can be used to synchronize phones (they know their offset with the crownstones).

After sending a switch command, the timestamp of the state can be used to ignore any state with a timestamp that has a smaller timestamp than the time (maybe plus some extra mesh delay) at which the command was sent.

To make room for the timestamp, we can reduce the size of the power factor to 1B. Also, we can send only the least significant part of the timestamp over the mesh and service data. And the least significant part of the energy used over the mesh.

We add a message type to the mesh state items to allow for more specialized types in the future.


#### New service data packet:

Type | Name | Length | Description
--- | --- | --- | ---
uint 8 | Type | 1 | Type of packet.
uint 16 | Crownstone ID | 2 | ID that identifies this Crownstone.
uint 8 | [Switch state](#switch_state_packet) | 1 | The state of the switch.
uint 8 | [Flags bitmask](#event_bitmask) | 1 | Bitflags to indicate a certain state of the Crownstone.
int 8 | Temperature | 1 | Chip temperature (°C).
int 8 | Power factor | 1 | The power factor at this moment. Divide by 127 to get the actual power factor.
int 16 | Power usage | 2 | The real power usage at this moment. Divide by 8 to get power usage in Watt. Divide by the power factor to get apparent power usage in VA.
int 32 | Energy used | 4 | The total energy used. Multiply by 64 to get the energy used in Joule.
uint16 | Partial timestamp | 2 | The least significant bytes of the timestamp. Meaning of timestamp depends on type.
uint 8 | Rand | 1 | Random byte.

Types:

Type | Description
---- | ----
0 | Timestamp is the time when this state was sent.
1 | Timestamp is the time when the switch was last changed.
2 | Timestamp is the time when the power usage or power factor last changed significantly.


#### New mesh state item:

Type | Name | Length | Description
--- | --- | --- | ---
uint 8 | Type | 1 | Type of item.
uint 16 | Crownstone ID | 2 | The identifier of the crownstone which has this state.
uint 8 | Switch state | 1 | The current [Switch state](#switch_state_packet) of the crownstone.
uint 8 | Event bitmask | 1 | The current [Event bitmask](#event_bitmask) of the crownstone.
int 8 | Power factor | 1 | The power factor at this moment. Divide by 127 to get the actual power factor.
int 16 | Power usage | 2 | The real power usage at this moment. Divide by 8 to get power usage in Watt. Divide by the power factor to get apparent power usage in VA.
uint 16 | Partial energy used | 2 | The least significant bytes of the energy used.
uint 16 | Partial timestamp | 2 | The least significant bytes of the timestamp. Meaning of timestamp depends on type.

Types:

Type | Description
---- | ----
0 | Timestamp is the time when this state was sent.
1 | Timestamp is the time when the switch was last changed.
2 | Timestamp is the time when the power usage or power factor last changed significantly.


## State variable size and resolution

We are most interested in the real power, so that gets the most bits. To get the apparent power, you multiply the real power with the power factor, thus the apparent power will have a resolution equal to power factor resolution.

### Power usage (real)

Should be able to represent a number from -3840 to 3840 (16A * 240V = 3840W).

Currently: int16 in units of 1/8W (2^15 / 8 = 4096)

Better: use some exponential representation, precision is less needed for high power usage.

### Power factor

Should be able to represent a decimal number from -1 to +1.

Currently: int8 in units of 1/127

Better: ??



### Energy used

Should be able to accumulate power usage over 10 years.

Currently: int32 in units of 64J (2^31 / (3600 x 24 x 365 x 10) x 64 = 435W on average)

Better: ??

### Partial timestamp

Should be able to figure out the time of a crownstone given that you know the current time.

Currently: 2 bytes in units of seconds. (2**15 / (3600) = 9 hour time drift before you compensate the wrongly)

Better: ??