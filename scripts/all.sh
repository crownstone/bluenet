#!/bin/sh

./softdevice.sh all
./firmware.sh build crownstone
./firmware.sh upload crownstone
./firmware.sh bootloader
