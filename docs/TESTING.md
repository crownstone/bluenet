# Firmware test

Before releasing a new firmware version, several tests must be performed.

## Soft fuses



### Current soft fuse

The current soft fuse should turn off the switch in case of over current. The default thresholds can be found in `Config.h`.


#### Test plan current soft fuse when using IGBTs

- Factory reset crownstone.
- Setup crownstone.
- Turn relay off.
- Turn dimmer on (at 100%).
- Plug in a load of 120W.
- Check if this continues to stay on for hours.

Now overload the IGBTs:

- Plug in a load of 300W.
- Check if this turns off immediately.
- Check if it reports the correct error state (`10` according to the [protocol](PROTOCOL.md#state_error_bitmask)).
- Check if you can't turn on the dimmer anymore.
- Check if you can still turn on the relay.

Now repeat the process below a couple of times with 300W and a couple of times with 2000W:

- Plug out the load.
- Reset the error state (or reset the crownstone).
- Turn dimmer on (at 100%).
- Plug in the load.
- Check if this turns off immediately.
- Check if it reports the correct error state (`10` according to the [protocol](PROTOCOL.md#state_error_bitmask)).



### Temperature soft fuse

The temperature soft fuse makes sure the Crownstone doesn't overheat. It monitors the temperature of the chip and the dimmer (IGBTs).
The default chip temperature threshold is set in the `CMakeBuild.config.default`, while the dimmer temperature threshold is defined in the `board.c` file.


#### Test plan chip temperature soft fuse

- Factory reset crownstone.
- Setup crownstone.
- Turn relay on.
- Plug in a load of 3000W, or blow hot air on the chip.

Now it should heat up (you should see the chip temperature rise) and turn off within an hour.

- Check if it turns off.
- Check if it reports the correct error state (`100` according to the [protocol](PROTOCOL.md#state_error_bitmask)).
- Check if you can't turn on the relay anymore.
- Check if you can't turn on the dimmer anymore.

Repeat the process a few times:

- Wait for it to cool off.
- Reset the error state (or reset the crownstone).
- Turn relay on.
- Check if it turns off.
- Check if it reports the correct error state (`100` according to the [protocol](PROTOCOL.md#state_error_bitmask)).
- Check if you can't turn on the relay anymore.
- Check if you can't turn on the dimmer anymore.


#### Test plan dimmer temperature soft fuse

- Factory reset crownstone.
- Setup crownstone.
- Turn relay off.
- Set current threshold dimmer at 16000mA.
- Reset crownstone.
- Verify current threshold dimmer is set at 16000mA.
- Turn dimmer on (at 100%).
- Plug in a load of 200W.
- Check if this continues to stay on for hours.

Now overload the dimmer:

- Plug in load of 300W.

Now it should heat up (you should see the chip temperature rise) and turn off within 15 minutes. If this doesn't happen, try with a load of 400W, or else 500W.

- Check if it turns off.
- Check if it reports the correct error state (`1000` according to the [protocol](PROTOCOL.md#state_error_bitmask)).
- Check if you can't turn on the relay anymore.
- Check if you can't turn on the dimmer anymore.
- Factory reset the crownstone.
