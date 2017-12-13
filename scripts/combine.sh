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

target=${1:-crownstone}

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh
source ${path}/_check_targets.sh $target
source $path/_config.sh

cd ${BLUENET_BIN_DIR} && rm -f combined*

add_bootloader=true
add_softdevice=true
add_binary=true

if [[ "$add_bootloader" == true ]]; then
	# These settings are already incorporated in the bootloader.hex binary, so you don't need to add them here
	#BOOTLOADER_SETTINGS="-exclude 0x3FC00 0x3FC20 -generate 0x3FC00 0x3FC04 -l-e-constant 0x01 4 -generate 0x3FC04 0x3FC08 -l-e-constant 0x00 4 -generate 0x3FC08 0x3FC0C -l-e-constant 0xFE 4 -generate 0x3FC0C 0x3FC20 -constant 0x00"

	ADD_BOOTLOADER="bootloader.hex -intel"
fi

if [[ "$SOFTDEVICE_SERIES" == "110" ]]; then
	if [[ "$add_softdevice" == true ]]; then
		ADD_SOFTDEVICE="${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_HEX}/${SOFTDEVICE}_softdevice.hex -intel"
	fi
	if [[ "$add_binary" == true ]]; then
		ADD_BINARY="crownstone.bin -binary -offset 0x00016000"
	fi
else
	if [[ "$add_softdevice" == true ]]; then
		ADD_SOFTDEVICE="${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_HEX}/${SOFTDEVICE}_softdevice.hex -intel"
	fi
	if [[ "$add_binary" == true ]]; then
		ADD_BINARY="crownstone.bin -binary -offset $APPLICATION_START_ADDRESS"
	fi
fi

cs_log "srec_cat $ADD_SOFTDEVICE $ADD_BOOTLOADER $ADD_BINARY $BOOTLOADER_SETTINGS -o combined.hex -intel"
srec_cat $ADD_SOFTDEVICE $ADD_BOOTLOADER $ADD_BINARY $BOOTLOADER_SETTINGS -o combined.hex -intel
res=$?
if [ ! $res -eq 0 ]; then
	cs_log "ERROR: srec_cat failed, check the results"
	exit 1
fi

cs_log "Combined binary is: ${BLUENET_BIN_DIR}/combined.hex"
cs_log "Done!"

