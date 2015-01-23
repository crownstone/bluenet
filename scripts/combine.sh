#!/bin/bash

cd ../build

#BOOTLOADER_SETTINGS="-exclude 0x3FC00 0x3FC20 -generate 0x3FC00 0x3FC04 -l-e-constant 0x01 4 -generate 0x3FC04 0x3FC08 -l-e-constant 0x00 4 -generate 0x3FC08 0x3FC0C -l-e-constant 0xFE 4 -generate 0x3FC0C 0x3FC20 -constant 0x00"

# in case you want to have a bootloader as well, uncomment the following
#ADD_BOOTLOADER="dobots_bootloader_xxaa.hex -intel"

# in case you want to use the S130
#ADD_SOFTDEVICE="/opt/softdevices/s130_nrf51822_0.5.0-1.alpha_softdevice.hex -intel"
#ADD_BINARY="crownstone.bin -binary -offset 0x0001c000"

# in case you want to use the S110
ADD_SOFTDEVICE="/opt/softdevices/s110_nrf51822_7.0.0_softdevice.hex -intel"
ADD_BINARY="crownstone.bin -binary -offset 0x00016000"

rm -f combined*

#echo srec_cat $ADD_SOFTDEVICE $ADD_BOOTLOADER $ADD_BINARY $BOOTLOADER_SETTINGS -o combined.hex -intel
echo "srec_cat $ADD_SOFTDEVICE $ADD_BOOTLOADER $ADD_BINARY $BOOTLOADER_SETTINGS -o combined.bin"
srec_cat $ADD_SOFTDEVICE $ADD_BOOTLOADER $ADD_BINARY $BOOTLOADER_SETTINGS -o combined.bin -binary

