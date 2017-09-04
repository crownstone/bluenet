# Firmware test

Before releasing a new firmware version, several tests must be performed.

## Soft fuse

### Current soft fuse

The current soft fuse should turn off the switch in case of over current. The default thresholds can be found in `Config.h`.

##### Test plan current soft fuse when using IGBTs

- Factory reset crownstone.
- Setup crownstone.
- Turn relay off.
- Turn dimmer on (at 100%).
- Plug in a load of 120W.

This should continue to stay on for hours.

- Now plug in a load of 300W.

This should turn off immediately, and the error state should be `10` (according to the [protocol](PROTOCOL.md#state_error_bitmask)).

Now repeat the proces below a couple of times with 300W and a couple of times with 2000W.

- Plug out the load.
- Reset the crownstone.
- Turn dimmer on (at 100%).
- Plug in the load.
- Check if it turns off and reports the correct error.

### Temperature soft fuse

The temperature soft fuse makes sure the Crownstone doesn't overheat. It monitors the temperature of the chip and the dimmer (IGBTs).
The default chip temperature threshold is set in the `CMakeBuild.config.default`, while the dimmer temperature threshold is defined in the `board.c` file.

##### Test plan chip temperature soft fuse

- Factory reset crownstone.
- Setup crownstone.
- Turn relay on.
- Plug in a load of 3000W.

Now it should heat up (you should see the chip temperature rise) and turn off within an hour.

Check if the error state is `100` (according to the [protocol](PROTOCOL.md#state_error_bitmask)).


##### Test plan dimmer temperature soft fuse

- Factory reset crownstone.
- Setup crownstone.
- Turn relay off.
- Set current threshold dimmer at 16000mA.
- Reset crownstone.
- Verify current threshold dimmer is set at 16000mA.
- Turn dimmer on (at 100%).
- Plug in load of 200W.

This should continue to stay on for hours.

- Plug in load of 300W.

Now it should heat up (you should see the chip temperature rise) and turn off within an hour. If this doesn't happen, try with a load of 400W, or else 500W.

Check if the error state is `1000` (according to the [protocol](PROTOCOL.md#state_error_bitmask)).

- Factory reset the crownstone.

