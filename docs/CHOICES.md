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

The buffers from the ADC are processed by the power sampling class. Since the zero of the voltage and current are not stable, these are first calculated by taking the average of a single period (20ms). This value is then smoothed by an exponential moving average.

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


# State propagation of Crownstones

## Problem

When using the mesh, a crownstone can advertise an old state of another crownstone, which can be advertised after the new state is advertised.

This leads to conflicting states, meaning the user can for example see the switch state toggling multiple times.

## More detailed problem statement

Currently there is a single timestamp for a complete message containing state information about the switch state, the power value, power factor, etc. Each time that any of these fields change, the timestamp is updated for the entire message. The timestamp henceforth destroys information about when the Crownstone has switched. This subsequently introduces all kind of race conditions. If you get a message with switch state information you can not rely on the corresponding timestamp. That timestamp is namely corrupted by any field change (and energy updates are for example every minute). 

To make it even more concrete. If the smartphone app receives a state message with a very recent timestamp T and state information X, Y, Z, it has no way to know if X, Y, or Z is new. It might very well be the case that X is extremely old and T refers only to Y. This means that even though the last time you turned on the Crownstones (state X) is a week ago, it still impossible for the smartphone app to use the information in the message to infer that X must be long ago. The smartphone app henceforth has to introduce additional latencies to make sure that race conditions are solved. For example waiting 10 seconds so it knows that its control messages have reached the target and that the state messages that have backpropagated are actually new. The problem is that we do not know if 10 seconds is sufficient or not. We might see all kind of inconsistencies depending on the mesh topology. However, if we take lost packets into account, we will need some kind of timeout regardless.

The main conceptual difference is that rather than seeing the Crownstones as sending out messages that define its state, this is seen as:
* A Crownstone sends out at regular intervals information about states or events. It is a good idea to use an event representation (state + timestamp) in the case there is a potential race condition.
* A Crownstone can send out different types of messages. It is not wise to summarize its entire state in one message. We need so-called opcodes in which a mesh network can be configured in such a way that more priority can be put on for example information transfer of energy messages rather than other types of messages. 

An example of other type of messages that might benefit from an event representation is that of people entering or leaving a room. As soon as we start implementing Crownstones scanning for iBeacons, this information might be useful to obtain straight from the Crownstone network itself rather than from the cloud.

If we assume that messages can be lost, the difference between state and event representation becomes smaller. In both cases we might want to wait say 10 seconds before we decide that a message apparently did not arrive at its destiny. In that case the toggle is reset back to "off" if an "on" message was sent. The main advantage of an event representation in this case is that every incoming message can be used to adjust the state. This means that we can easily set this delay to 5 minutes. If we then in the meantime get an incoming message about an state change to "off" while we sent ourselves an "on" message due to someone else sending a message, we can properly react to this. We do not need to wait 5 minutes before we can react to state messages. However, in the case that the command has been superseded by another command to a different Crownstone, the result would be a 5 minute delay. We will need to implement merging of multiswitch commands to avoid this. Even if this is the case, it can happen that more Crownstones than the capacity of the multiswitch packet are switched, making it impossible to merge. Taking all these cases into account, a 5 minute delay is unreasonable.

Note, that this also assumes a basic form of time synchronization is implemented. At https://www.cse.wustl.edu/~jain/cse574-06/ftp/time_sync/index.html you will see many advantages of having time (or more precise clock) synchronization, amongst which are: localization, proximity, and energy efficiency.

## Mesh state item size

The list of state items in the mesh state message, currently holds place for 2x7 items, 12 bytes each.

A larger item size holds more data per item, which means less messages need to be sent in order to transmit all data. Less messages means less conflicts on the mesh.

On the other hand, the state of less Crownstones will fit in a message, so if too many Crownstones send their state at the same time, the state of certain Crownstones will be pushed out of the state message. This will most likely happen when Crownstones are being switched at the same time.

## Considerations

1. When entering sphere, phone needs to get state from all crownstones.
2. Phone shouldn't show old state when:
    1. When toggling switch.
    2. When toggling switch multiple times (can still go wrong with current implementation, depends on mesh delay).
3. Multiple user with phones which are out of sync (more than 1s).

## Proposal

Assumption: the crownstones clocks are synchronized (on the second). This should be the case when the clocks are regularly (daily?) set by the phone via the mesh.

The proposal is to add timestamps to [state items](PROTOCOL.md#state-mesh-item) on the mesh message, as well as timestamps to the [advertisement](PROTOCOL.md#scan-response-servicedata-packet).

The advertised timestamp can be used to synchronize phones (they know their offset with the crownstones).

After sending a switch command, the timestamp of the state can be used to ignore any state with a timestamp that has a smaller timestamp than the time (maybe plus some extra mesh delay) at which the command was sent.

To make room for the timestamp, we can reduce the size of the power factor to 1B. Also, we can send only the least significant part of the timestamp over the mesh and service data. And the least significant part of the energy used over the mesh.

We add a message type to the mesh state items to allow for more specialized types in the future.

We increase the size of the mesh state item to 14 bytes, which decreases the number of items to 2x6. This makes the state item size closer to the service data size (16B), so that the mesh state can be advertised 1 to 1, while the chance of 12 crownstones having to send their state is rather small.

### Flags Bitmask

Bit | Name |  Description
--- | --- | ---
0 | Dimming available | When dimming is physically available, this will be 1.
1 | Marked as dimmable | When dimming is configured to be allowed, this will be 1.
2 | Error |  If this is 1, the Crownstone has an error, you can check what error it is in the error service data, or by reading the [error state](#state-packet).
3 | Switch locked | When the switch state is locked, this will be 1.
4 | Reserved | Reserved for future use.
5 | Reserved | Reserved for future use.
6 | Reserved | Reserved for future use.
7 | Reserved | Reserved for future use.



### New mesh state item:

Type | Name | Length | Description
--- | --- | --- | ---
uint 8 | Type | 1 | 0 (current state)
uint 8 | Crownstone ID | 1 | The identifier of the crownstone which has this state.
uint 8 | [Switch state](#switch-state-packet) | 1 | The state of the switch.
uint 8 | [Flags bitmask](#flags-bitmask) | 1 | Bitflags to indicate a certain state of the Crownstone.
int 8 | Temperature | 1 | Chip temperature (°C).
int 8 | Power factor | 1 | The power factor at this moment. Divide by 127 to get the actual power factor.
int 16 | Power usage | 2 | The real power usage at this moment. Divide by 8 to get power usage in Watt. Divide real power usage by the power factor to get apparent power usage in VA.
int 32 | Energy used | 4 | The total energy used. Multiply by 64 to get the energy used in Joule.
uint 16 | Partial timestamp | 2 | The least significant bytes of the timestamp when this was the state of the Crownstone.


Type | Name | Length | Description
--- | --- | --- | ---
uint 8 | Type | 1 | 1 (error)
uint 8 | Crownstone ID | 1 | The identifier of the crownstone which has this state.
uint 32 | [Error bitmask](#state-error-bitmask) | 4 | Error bitmask of the Crownstone.
uint 32 | Timestamp | 4 | The timestamp when the first error occurred.
uint 8 | [Flags bitmask](#flags-bitmask) | 1 | Bitflags to indicate a certain state of the Crownstone.
int 8 | Temperature | 1 | Chip temperature (°C).
uint 16 | Partial timestamp | 2 | The least significant bytes of the timestamp when this were the flags and temperature of the Crownstone.


Type | Name | Length | Description
--- | --- | --- | ---
uint 8 | Type | 1 | 2 (last events)
uint 8 | Crownstone ID | 1 | The identifier of the crownstone which has this state.
uint 8 | [Switch state](#switch-state-packet) | 1 | The state of the switch.
uint 32 | Timestamp | 4 | The timestamp when the switch last changed.
int 8 | Power factor | 1 | The power factor at this moment. Divide by 127 to get the actual power factor.
int 16 | Power usage | 2 | The real power usage at this moment. Divide by 8 to get power usage in Watt. Divide real power usage by the power factor to get apparent power usage in VA.
uint 32 | Timestamp | 4 | The timestamp when the power usage last changed significantly.


### New service data packet:

Protocol version: use 3 for encrypted data, 4 for a setup packet, 5 for dfu, 6 for unencrypted/guest.

#### Normal mode

The following type sends the current (last) state of the Crownstone, this will be sent most of the times.
The validation value helps to validate, and thus discover Crownstones more quickly when entering a sphere.

Type | Name | Length | Description
--- | --- | --- | ---
uint 8 | Unencrypted opcode | 1 | 3 (encrypted data)
uint 8 | Type | 1 | 0 (state)
uint 8 | Crownstone ID | 1 | ID that identifies this Crownstone.
uint 8 | [Switch state](#switch-state-packet) | 1 | The state of the switch.
uint 8 | [Flags bitmask](#flags-bitmask) | 1 | Bitflags to indicate a certain state of the Crownstone.
int 8 | Temperature | 1 | Chip temperature (°C).
int 8 | Power factor | 1 | The power factor at this moment. Divide by 127 to get the actual power factor.
int 16 | Power usage | 2 | The real power usage at this moment. Divide by 8 to get power usage in Watt. Divide real power usage by the power factor to get apparent power usage in VA.
int 32 | Energy used | 4 | The total energy used. Multiply by 64 to get the energy used in Joule.
uint 16 | Partial timestamp | 2 | The least significant bytes of the timestamp when this was the state of the Crownstone.
uint 16 | Validation | 2 | Value is always `0xFACE`. Can be used to help validating that the decryption was successful.


The following type only gets advertised in case there is an error. It will be interleaved with the state type.

Type | Name | Length | Description
--- | --- | --- | ---
uint 8 | Unencrypted opcode | 1 | 3 (encrypted data)
uint 8 | Type | 1 | 1 (error)
uint 8 | Crownstone ID | 1 | The identifier of the crownstone which has this state.
uint 32 | [Error bitmask](#state-error-bitmask) | 4 | Error bitmask of the Crownstone.
uint 32 | Timestamp | 4 | The timestamp when the first error occurred.
uint 8 | [Flags bitmask](#flags-bitmask) | 1 | Bitflags to indicate a certain state of the Crownstone.
int 8 | Temperature | 1 | Chip temperature (°C).
uint 16 | Partial timestamp | 2 | The least significant bytes of the timestamp when this were the flags and temperature of the Crownstone.
int 16 | Power usage | 2 | The real power usage at this moment. Divide by 8 to get power usage in Watt. Divide real power usage by the power factor to get apparent power usage in VA.


The following type sends out the last known state of another Crownstone. It will be interleaved with the state type (unless there's an error).

Type | Name | Length | Description
--- | --- | --- | ---
uint 8 | Version | 1 | 3 (encrypted data)
uint 8 | Type | 1 | 2 (external state)
uint 8 | External Crownstone ID | 1 | The identifier of the crownstone which has the following state.
uint 8 | [Switch state](#switch-state-packet) | 1 | The state of the switch.
uint 8 | [Flags bitmask](#flags-bitmask) | 1 | Bitflags to indicate a certain state of the Crownstone.
int 8 | Temperature | 1 | Chip temperature (°C).
int 8 | Power factor | 1 | The power factor at this moment. Divide by 127 to get the actual power factor.
int 16 | Power usage | 2 | The real power usage at this moment. Divide by 8 to get power usage in Watt. Divide real power usage by the power factor to get apparent power usage in VA.
int 32 | Energy used | 4 | The total energy used. Multiply by 64 to get the energy used in Joule.
uint 16 | Partial timestamp | 2 | The least significant bytes of the timestamp when this was the state of the Crownstone.
uint 8 | Reserved | 2 | Reserved for future use.


The following type sends out the last known error of another Crownstone. It will be interleaved with the state type (unless there's an error).

Type | Name | Length | Description
--- | --- | --- | ---
uint 8 | Version | 1 | 3 (encrypted data)
uint 8 | Type | 1 | 3 (external error)
uint 8 | External Crownstone ID | 1 | The identifier of the crownstone which has the following state.
uint 32 | [Error bitmask](#state-error-bitmask) | 4 | Error bitmask of the Crownstone.
uint 32 | Timestamp | 4 | The timestamp when the first error occurred.
uint 8 | [Flags bitmask](#flags-bitmask) | 1 | Bitflags to indicate a certain state of the Crownstone.
int 8 | Temperature | 1 | Chip temperature (°C).
uint 16 | Partial timestamp | 2 | The least significant bytes of the timestamp when this were the flags and temperature of the Crownstone.
uint 8 | Reserved | 2 | Reserved for future use.


#### Setup mode

When in setup mode, an unencrypted state is sent. The Crownstone ID is not set yet, and the timestamp is not needed, as the Crownstone will not be in a mesh yet. The energy used is not kept up, as nothing is stored yet. This creates space to also include the error bitmask.

The counter is required for the service data to keep changing. Else, the phone might ignore the service data, as it was seen before.

Type | Name | Length | Description
--- | --- | --- | ---
uint 8 | Version | 1 | 4 (setup mode)
uint 8 | Type | 1 | 0 (state)
uint 8 | [Switch state](#switch-state-packet) | 1 | The state of the switch.
uint 8 | [Flags bitmask](#flags-bitmask) | 1 | Bitflags to indicate a certain state of the Crownstone.
int 8 | Temperature | 1 | Chip temperature (°C).
int 8 | Power factor | 1 | The power factor at this moment. Divide by 127 to get the actual power factor.
int 16 | Power usage | 2 | The real power usage at this moment. Divide by 8 to get power usage in Watt. Divide real power usage by the power factor to get apparent power usage in VA.
uint 32 | [Error bitmask](#state-error-bitmask) | 4 | Error bitmask of the Crownstone.
uint 8 | Counter | 1 | Simply counts up and overflows.
uint 8 | Reserved | 4 | Reserved for future use.



## State variable size and resolution

### Crownstone ID

With the current mesh, we can probably not support more than 50 Crownstones, so 255 different ids should be plenty.

### Power usage (real)

We are most interested in the real power, so that gets the most bits. To get the apparent power, you divide the real power with the power factor, thus the apparent power will have a resolution equal to power factor resolution.

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

### Partial energy used

Should be able to figure out the energy used, given that you know the energy used of some time ago.

Currently: none, 2 least significant bytes can overflow in 5 minutes, 3 bytes can overflow in a day.

### Partial timestamp

Should be able to figure out the time of a crownstone given that you know the current time.

Currently: 2 bytes in units of seconds. (2**15 / (3600) = 9 hour time drift before you compensate the wrongly)

Better: ??

### Compressed timestamp

Should be able to represent an absolute time for the last switch time etc.

Currently: none, as it should be able to have seconds precision, but also be able to represent a certain day.

Idea: use 3 least significant bytes, that only overflows in 97 days.

Idea: in combination with partial timestamp: have a 2 byte float that represents the offset from the partial timestamp.

# Multiswitch Merging

## Problem

Assume a mesh delay of 3 seconds. If Alice switches CS1 on and a second later Bob switches CS2 on, there is a race condition where Bob's command will replace Alice's command before it reaches CS1. This means Alice will wait on a switch of CS1 which will never happen.

## Proposal

Merge conflicting messages in the mesh. A conflict occurs when one Crownstone receives two mesh messages with the same version. Resolving this conflict has to be deterministic so every Crownstone with this conflict will resolve it to the same message, avoiding propagated conflicts.

Rules for merger:

- On is more important than Off.
- Sort list by Crownstone ID.


# Switching, locking and dimming

If a Crownstone is dimmable, generally speaking it's relay is off and the IGBTs are on. If the Crownstone is reset (wall switch), the relay goes on quickly, but after a minute (due to powersupply on the IGBTs) it toggles off again to set the dim value. Since this change after a minute is more strange than it is helpful, I propose to NOT to persist the dimstate after reboot. This makes for a clearer user experience.

Alternatively, we could change this (later on, possibly) to fade to the last known dim value as long as it is significant ( less than 75% for instance). If there is a click after a minute that will not change a lot, I suggest we ignore it.

### Locking

Locking is added for the usecase of using Crownstones are power monitor on devices that are unlikely to every be turned off forcefully: Fridges, PCs etc. It does not make sense to lock a Crownstone in a dimming state. I propose to only allow locking for non-dimmable Crownstones.

If we were to allow locking for dimmed states, they would have to persist after reset (from a wall socket) for consistency. This would interfere with the expected result of 1 minute full on and then back to dim state, leading to a bad user experience.

### Cases

- On boot, turn relay on when stored state was dimming, don't restore dim state.
- When setting dim value before dimming is available, this is stored and set once dimming is available.
- If lock is enabled, and dimming gets enabled: then disable lock.
- If dimming is enabled, and lock gets enabled: deny enabling lock.

