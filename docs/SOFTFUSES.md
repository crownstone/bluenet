# Softfuses
-----------
The Crownstone has several software fuses implemented.

## Over current
Prevent the whole Crownstone from overheating, caused by too much current going through the relay.

#### Trigger
The Crownstone measures too much current, while the relay is on.

#### Default threshold
16A

#### Action
- Dimmer off
- Relay off

#### False positives/negatives

Cause | Likeliness | Explanation
----- | ---------- | -----------
There is a bug in the current measurement code. | low | This should quickly be found out during tests.
Noise on the current measurement. | very low | Getting to 16A will need a lot of noise.



## Dimmer over current
Prevent the dimmer (IGBTs) from overheating, caused by too much current going through the dimmer.

#### Trigger
The Crownstone measures too much current, while the relay is off, and the dimmer on.

#### Default threshold
1A

#### Action
- Dimmer off
- Relay on

#### False positives/negatives

Cause | Likeliness | Explanation
----- | ---------- | -----------
There is a bug in the current measurement code. | low | This should quickly be found out during tests.
Noise on the current measurement. | medium | Getting to 1A will a substantial amount of noise. Nonetheless, this has been observed when the Crownstone is very near a microwave for example.



## Chip temperature
Prevent the whole Crownstone from overheating, caused by too much current going through the relay.

#### Trigger
The chip temperature of Crownstone is too high.

#### Default threshold
75°C

#### Action
- Dimmer off
- Relay off

#### False positives/negatives

Cause | Likeliness | Explanation
----- | ---------- | -----------
There is a bug in the chip temperature measurement code. | very low | This is a very simple implementation.
Situation


## Dimmer temperature
Prevent the dimmer (IGBTs) from overheating, caused by too much current going through the dimmer.

#### Trigger
The dimmer temperature of Crownstone is too high.

#### Default threshold
Depends on Crownstone type.

#### Action
- Dimmer off
- Relay on

#### False positives/negatives

Cause | Likeliness | Explanation
----- | ---------- | -----------
There is a bug in the dimmer temperature measurement code. | very low | This is a very simple implementation.
Not the dimmer, but the environment is too hot. | low | Since the dimmer thermometer can be relatively far, the threshold can be low, to compensate for the distance.



## Dimmer on failure
Detect a broken dimmer, one that's always on.

#### Trigger
The dimmer and relay are off, but there is still current measured.

#### Default threshold
1A

#### Action
- Dimmer off
- Relay on

#### False positives/negatives

Cause | Likeliness | Explanation
----- | ---------- | -----------
There is a bug in the current measurement code. | low | This should quickly be found out during tests.
Noise on the current measurement. | medium | Getting to 1A will a substantial amount of noise. Nonetheless, this has been observed when the Crownstone is very near a microwave for example.



## Dimmer off failure
Detect a broken dimmer, one that's always off.

#### Trigger
Not implemented.