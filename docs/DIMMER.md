# Dimmer
--------

The Crownstone can act as a regular dimmer. It does so by switching the power on and off very quickly (100Hz). It's best explained visually: ![trailing edge dimmer](images/ideal_dimmer.gif "trailing edge dimmer")

### Dimmer type

Currently it's a trailing edge dimmer. In short, this means it turns on when there is 0V, and turns off when there is voltage. This is usually better for the life-time of lights, as there is no sudden inrush of electricity.

### Compatibility

All dimmable lights that we tested, worked great! We keep up a list of lights [here](https://crownstone.rocks/compatibility/dimming/).

### Persistence

The state of the dimmer is saved, so that when you turn the Crownstone off and on again, the dimmer will be at same percentage again.
Unfortunately, it currently takes about 2 seconds before the dimmer state is restored, as it takes some time for the internal power supply to initialize. However, this delay can be reduced by software improvements later on.
