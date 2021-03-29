# What to do in case something goes wrong at home
------------------------------

If something goes wrong with crownstones at home, this document describes what you can do to help debugging the firmware.

This document is aimed at team members and developers.

## Hardware error

If you get a popup that something is wrong with one of your crownstones:

- Take a screenshot of the popup.
- Get the firmware version of the crownstone (in the edit menu of the crownstone).
- Get softfuse buffers.
- Turn crownstone on.
- Get filtered, and unfiltered buffers.
- Share all buffers (via dev menu). Mention what load is plugged in, where the crownstone is located.

## Unexpected change of switch state.

If a crownstone turns on/off when you wouldn't expect it, or it didn't switch while you expected it:

- Get the firmware version of the crownstone (in the edit menu of the crownstone).
- Get switch history, maybe this already explains it?
- Get behaviour debug, maybe this already explains it?
- Get uptime.
    - If the uptime is low, get last reset reason.
- Share logs (via dev menu) with an explanation of what happened and what you expected to happen.


