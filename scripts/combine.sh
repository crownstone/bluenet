#!/bin/bash

#######################################################################################################################
# Important notice!
# - For over-the-air programming, you will need .hex files.
# - For the ST-Link programmer, you will need them as well. Especially for the bootloader. The 0x10001014 field for
#   example is part of the .hex file.
#
# Copyrights:
#   Author: Anne van Rossum
#   Company: Distributed Organisms B.V. (https://dobots.nl)
#   Date: Feb. 3, 2014
#######################################################################################################################

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/config.sh

cd ${BLUENET_CONFIG_DIR}/build && rm -f combined*

add_bootloader=false
add_softdevice=true
add_binary=true

if [[ "$add_bootloader" == true ]]; then
	# These settings are already incorporated in the bootloader.hex binary, so you don't need to add them here
	#BOOTLOADER_SETTINGS="-exclude 0x3FC00 0x3FC20 -generate 0x3FC00 0x3FC04 -l-e-constant 0x01 4 -generate 0x3FC04 0x3FC08 -l-e-constant 0x00 4 -generate 0x3FC08 0x3FC0C -l-e-constant 0xFE 4 -generate 0x3FC0C 0x3FC20 -constant 0x00"
	
	ADD_BOOTLOADER="bootloader.hex -intel"
fi

if [[ "$SOFTDEVICE_SERIES" == "110" ]]; then
	if [[ "$add_softdevice" == true ]]; then
		ADD_SOFTDEVICE="${SOFTDEVICE_DIR}/${SOFTDEVICE}_softdevice.hex -intel"
	fi
	if [[ "$add_binary" == true ]]; then
		ADD_BINARY="crownstone.bin -binary -offset 0x00016000"
	fi
else
	if [[ "$add_softdevice" == true ]]; then
		ADD_SOFTDEVICE="${SOFTDEVICE_DIR}/${SOFTDEVICE}_softdevice.hex -intel"
	fi
	if [[ "$add_binary" == true ]]; then
		ADD_BINARY="crownstone.bin -binary -offset 0x0001c000"
	fi
fi

echo "srec_cat $ADD_SOFTDEVICE $ADD_BOOTLOADER $ADD_BINARY $BOOTLOADER_SETTINGS -o combined.hex -intel"
srec_cat $ADD_SOFTDEVICE $ADD_BOOTLOADER $ADD_BINARY $BOOTLOADER_SETTINGS -o combined.hex -intel
res=$?
if [ ! $res -eq 0 ]; then
	echo "ERROR: srec_cat failed, check the results"
	exit 1
fi

#echo "Now run:"
#echo "./flash_openocd.sh combined"
#echo "this will place the binary at location 0 of FLASH memory of the target device"
echo "Combined binary is: ${BLUENET_CONFIG_DIR}/build/combined.hex"
echo "Done!"

