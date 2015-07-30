#!/bin/sh
#
# Parameter is optional, provide target if wanted. If left out, default target is crownstone

./softdevice.sh all $1
./firmware.sh build $1
./firmware.sh upload $1
./firmware.sh bootloader $1
