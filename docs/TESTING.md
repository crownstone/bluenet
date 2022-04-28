# Firmware test

Before releasing a new firmware version, several tests must be performed.



# Soft fuses

## Current soft fuse

The current soft fuse should turn off the switch in case of over current. The default thresholds can be found in `Config.h`.


### Test plan current soft fuse when using IGBTs

First see if the dimmer stays on (checking for false negatives):

- Factory reset crownstone.
- Setup crownstone.
- Turn relay off.
- Enable dimming.
- Turn dimmer on (at 100%).
- Plug in a load of 120W - 150W.
- Place a phone right next to it, call it every 5 minutes for the first hours.
- [ ] Check if this continues to stay on for hours.

Repeat, but now with actual dimming:

- Turn relay off.
- Enable dimming.
- Turn dimmer on at 50%.
- Plug in a load of 120W - 150W.
- Place a phone right next to it, call it every 5 minutes for the first hours.
- [ ] Check if this continues to stay on for hours.


Next, overload the IGBTs:

- Enable dimming.
- Turn dimmer on (at 100%).
- Enable switch lock.
- Plug in a light (so you can see when the dimmer turns off).
- Additionally, plug in a load of 300W.
- [ ] Check if the relay turns on immediately.
- [ ] Check if it reports the correct error state (`10` according to the [protocol](protocol/PROTOCOL.md#state_error_bitmask)).
- Disable switch lock.
- [ ] Check if you can't turn on the dimmer anymore.
- [ ] Check if you can't turn off the relay anymore.


Now repeat the process below a couple of times with 300W and a couple of times with 2000W:

- Plug out the load.
- Reset the error state (or reset the crownstone).
- Turn dimmer on (at 100%).
- Enable switch lock.
- Plug in the load.
- [ ] Check if the relay turns on immediately.
- [ ] Check if it reports the correct error state (`10` according to the [protocol](protocol/PROTOCOL.md#state_error_bitmask)).
- Disable switch lock.

Now try it in setup mode:

- Factory reset crownstone.
- Enable dimming.
- Turn dimmer on.
- Plug in a load of 300W or more.
- [ ] Check if the relay turns on immediately.
- [ ] Check if it reports the correct error state (`10` according to the [protocol](protocol/PROTOCOL.md#state_error_bitmask)).
- [ ] Check if you can't turn on the dimmer anymore.
- [ ] Check if you can't turn off the relay anymore.
- Factory reset the crownstone.


## Temperature soft fuse

The temperature soft fuse makes sure the Crownstone doesn't overheat. It monitors the temperature of the chip and the dimmer (IGBTs).
The default chip temperature threshold is set in the `CMakeBuild.config.default`, while the dimmer temperature threshold is defined in the `board.c` file.


### Test plan chip temperature soft fuse

- Factory reset crownstone.
- Setup crownstone.
- Enable dimming.
- Turn relay on.
- Enable switch lock.
- Plug in a load of 3000W, or blow hot air on the chip.

Now it should heat up (you should see the chip temperature rise) and turn off within an hour.

- [ ] Check if it turns off.
- [ ] Check if it reports the correct error state (`100` according to the [protocol](protocol/PROTOCOL.md#state_error_bitmask)).
- Disable switch lock.
- [ ] Check if you can't turn on the relay anymore.
- [ ] Check if you can't turn on the dimmer anymore.


Repeat the process a few times:

- Wait for it to cool off.
- Reset the error state (or reset the crownstone).
- Turn relay on.
- [ ] Check if it turns off.
- [ ] Check if it reports the correct error state (`100` according to the [protocol](protocol/PROTOCOL.md#state_error_bitmask)).
- [ ] Check if you can't turn on the relay anymore.
- [ ] Check if you can't turn on the dimmer anymore.


Now try it in setup mode:

- Wait for it to cool off.
- Factory reset crownstone.
- Enable dimming.
- Turn relay on.
- [ ] Check if it turns off.
- [ ] Check if it reports the correct error state (`100` according to the [protocol](protocol/PROTOCOL.md#state_error_bitmask)).
- [ ] Check if you can't turn on the relay anymore.
- [ ] Check if you can't turn on the dimmer anymore.


### Test plan dimmer temperature soft fuse

- Factory reset crownstone.
- Setup crownstone.
- Enable dimming.
- Turn relay off.
- Set current threshold dimmer at 16000mA.
- Reset crownstone.
- Verify current threshold dimmer is set at 16000mA.
- Turn dimmer on (at 100%).
- Plug in a load of 200W.
- [ ] Check if this continues to stay on for hours.

Now overload the dimmer:

- Enable switch lock.
- Plug in load of 300W.

Now it should heat up (you should see the chip temperature rise) and turn the relay on within 15 minutes. If this doesn't happen, try with a load of 400W, or else 500W.

- [ ] Check if the relay turns on.
- [ ] Check if it reports the correct error state (`1000` according to the [protocol](protocol/PROTOCOL.md#state_error_bitmask)).
- Disable switch lock.
- [ ] Check if you can't turn off the relay anymore.
- [ ] Check if you can't turn on the dimmer anymore.
- Factory reset the crownstone.


## IGBT failure detection

If one of the IGBTs failed, it will be always on or always off. The former case is the worst (as current will go through the IGBT when the relay is off), but also the easiest to detect.

### Test plan IGBT always on

- Get a crownstone with a broken IGBT.
- Factory reset the crownstone.
- Setup the crownstone.
- Enable dimming.
- Turn dimmer off.
- Turn relay off.
- Enable switch lock.
- Plug in load of 300W.
- [ ] Check if the relay turns on immediately.
- [ ] Check if it reports the correct error state (`10000` according to the [protocol](protocol/PROTOCOL.md#state_error_bitmask)).
- Disable switch lock.
- [ ] Check if you can't turn off the relay anymore.
- [ ] Check if you can't turn on the dimmer anymore.


Now try it in setup mode:

- Factory reset the crownstone.
- Enable dimming.
- Turn dimmer off.
- Turn relay off.
- Plug in load of 300W.
- [ ] Check if the relay turns on immediately.
- [ ] Check if it reports the correct error state (`10000` according to the [protocol](protocol/PROTOCOL.md#state_error_bitmask)).
- [ ] Check if you can't turn off the relay anymore.
- [ ] Check if you can't turn on the dimmer anymore.


# Switch

The switch can be locked, and to be able to dim, you have to enable dimming.
Next to that, dimming cannot be used immediately after boot, so the relay is temporarily used instead.


## Lock

Check if the lock works when powered on:

- Set switch on.
- [ ] Check if advertisement data reports the switch is not locked.
- Enable switch lock.
- [ ] Check if switching off does not work.
- [ ] Check if advertisement data reports the switch is locked.
- Disable switch lock.

Check if the lock works when powered on:

- Set switch off.
- Enable switch lock.
- [ ] Check if switching on does not work.
- Disable switch lock.

Check if lock works with dimming:

- Enable dimming.
- Set switch at 50.
- Enable switch lock.
- [ ] Check if switching on does not work.
- [ ] Check if switching off does not work.
- [ ] Check if you can't change the switch value anymore.
- Disable switch lock.


## Dimming available

Check if you can't use dimming immediately after boot:

- Enable dimming.
- Plug out crownstone.
- Plug in crownstone.
- Set switch at 30.
- [ ] Check if the relay turns on.
- [ ] Check if advertisement data reports dimming is not available.
- Wait 60s.
- [ ] Check if the relay turns off, and dimmer is set at 30.
- [ ] Check if advertisement data reports dimming is available.

Check if the last state is restored:

- Enable dimming.
- Plug out crownstone.
- Plug in crownstone.
- Set switch at 30.
- [ ] Check if the relay turns on.
- Set switch at 100.
- [ ] Check if the relay stays on after 60s.

Check if dimmer state restores:

- Enable dimming.
- Set switch at 25.
- Wait 10 seconds.
- Plug out crownstone.
- Plug in crownstone.
- [ ] Check if the relay turns on (write down how many seconds after plugging in the crownstone).
- [ ] Check if the relay turns off, and dimmer is set at 25, after 60s after boot.


## Dimming allowed

Basic:

- Disable dimming.
- [ ] Check if you cannot dim.

In combination with the lock:

- Enable dimming.
- [ ] Check if advertisement data reports dimming is allowed.
- Set switch at 30.
- Enable switch lock.
- Disable dimming.
- [ ] Check if the relay turns on, and dimmer off.
- [ ] Check if advertisement data reports dimming is not allowed.
- Wait 10 seconds.
- Plug out crownstone.
- Plug in crownstone.
- Wait 60s.
- [ ] Check if crownstone isn't dimming.
- Disable switch lock.

After boot:

- Enable dimming.
- Set switch at 30.
- Wait 10 seconds.
- Plug out crownstone.
- Plug in crownstone.
- Disable dimming immediately.
- Wait 60s.
- [ ] Check if crownstone isn't dimming.



# Dimmer

## Commands

Test if dimming works via different ways:

- Enable dimming.
- [ ] Dim via switch.
- [ ] Dim via IGBT.
- [ ] Dim via multiswitch.
- [ ] Dim via mesh.

## Stability

Check dimmer stability with:

- An incandescent lamp.
- A halogen lamp.
- A dimmable LED lamp.



# Power measurements

Check if the power measurements are ok.

- Repeat this process for multiple crownstones.
- For each check, measure about 30 seconds.
- For each check, write down how much difference there is between the measured power by the Crownstone and the external reference.
- For each check, write down how stable the output is (are there peaks?, how much noise in W?).

Off:

- Turn relay off, dimmer off.
- [ ] Check if measured power is about 0W.

Relay:

- Turn relay on, dimmer off.
- [ ] Check if measured power is about 0W.
- Plug in about 30W load
- [ ] Check if measured power is about right, write down results.
- Plug in about 60W load
- [ ] Check if measured power is about right, write down results.
- Plug in about 100W load
- [ ] Check if measured power is about right, write down results.
- Plug in about 800W load
- [ ] Check if measured power is about right, write down results.

Dimmer:

- Turn relay off
- Dimmer 100%
- [ ] Check if measured power is about 0W.
- Plug in about 30W load
- [ ] Check if measured power is about right, write down results.
- Plug in about 60W load
- [ ] Check if measured power is about right, write down results.
- Plug in about 100W load
- [ ] Check if measured power is about right, write down results.

Dimming:

- Relay off, load of about 30W.
- Dimmer at 25%
- [ ] Check if measured power is about right, write down results.
- Dimmer at 50%
- [ ] Check if measured power is about right, write down results.
- Dimmer at 75%
- [ ] Check if measured power is about right, write down results.



# Setup

For a plug:

- Turn relay off.
- Factory reset crownstone.
- Setup crownstone.
- [ ] Check if relay turns on when setup is complete.
- Factory reset crownstone.
- [ ] Check if relay turns off after factory reset.

For a built-in:

- Turn relay off.
- Factory reset crownstone.
- [ ] Check if relay turns on after 1 hour.
- Turn relay off.
- Setup crownstone.
- [ ] Check if relay turns on when setup is complete.
- Factory reset crownstone.
- [ ] Check if relay turns off after factory reset.
