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

usage() {
	echo "Combines multiple binaries into a single binary"
	echo
	echo "What to include:"
	echo "   -f, --firmware                       include firmware"
	echo "   -b, --bootloader                     include bootloader"
	echo "   -s, --softdevice                     include softdevice"
	echo "   -h, --hardware_verion                include hardware version"
	echo
	echo "Extra arguments:"
	echo "   -t target, --target target           specify config target (files are generated in separate directories)"
}

getopt --test > /dev/null
if [[ $? -ne 4 ]]; then
	echo "I’m sorry, `getopt --test` failed in this environment."
	exit $ERR_GETOPT_TEST
fi

SHORT=t:fbsh
LONG=target:firmware,bootloader,softdevice,hardware_version

PARSED=$(getopt --options $SHORT --longoptions $LONG --name "$0" -- "$@")
if [[ $? -ne 0 ]]; then
	exit $ERR_GETOPT_PARSE
fi

eval set -- "$PARSED"

while true; do
	case "$1" in
		-t|--target) 
			target=$2
			shift 2
			;;
		-f|--firmware)
			include_firmware=true
			shift 1
			;;
		-b|--bootloader)
			include_bootloader=true
			shift 1
			;;
		-s|--softdevice)
			include_softdevice=true
			shift 1
			;;
		-h|--hardware_version)
			include_hardware_version=true
			shift 1
			;;
		--)
			shift
			break
			;;
		*)
			echo "Error in arguments."
			echo $2
			exit $ERR_ARGUMENTS
			;;
	esac
done

target=${target:-crownstone}

# Use the current path as the bluenet directory
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

export BLUENET_DIR=$(readlink -m ${path}/..)

cs_info "Load configuration from: ${path}/_check_targets.sh $target"
source ${path}/_check_targets.sh $target

cs_info "Load configuration from: ${path}/_config.sh"
source $path/_config.sh

cd ${BLUENET_BIN_DIR} && rm -f combined*


if [[ $include_bootloader ]]; then
	# These settings are already incorporated in the bootloader.hex binary, so no need to add them here as well
	#BOOTLOADER_SETTINGS="-exclude 0x3FC00 0x3FC20 -generate 0x3FC00 0x3FC04 -l-e-constant 0x01 4 -generate 0x3FC04 0x3FC08 -l-e-constant 0x00 4 -generate 0x3FC08 0x3FC0C -l-e-constant 0xFE 4 -generate 0x3FC0C 0x3FC20 -constant 0x00"

	ADD_BOOTLOADER="bootloader.hex -intel"
fi

if [[ $include_hardware_version ]]; then
	BOARD_FILE="$BLUENET_DIR/include/cfg/cs_Boards.h"
	cs_info "Obtain hardware board from $BOARD_FILE"
	HARDWARE_BOARD_INT=$(grep -oP "#define\s+$HARDWARE_BOARD\s+\d+" $BOARD_FILE | grep -oP "\d+$")
	if [[ ! "$HARDWARE_BOARD_INT" ]]; then
		cs_err "ERROR: No hardware board defined (check target=$target in $BOARD_FILE)"
		exit 1
	fi
	HARDWARE_BOARD_HEX=$(printf "0x%08x" $HARDWARE_BOARD_INT)
	cs_info "Write $HARDWARE_BOARD_HEX to address: $HARDWARE_BOARD_ADDRESS"
	HARDWARE_BOARD_CONFIG="-exclude 0x10001084 0x10001088 -generate 0x10001084 0x10001088 -l-e-constant $HARDWARE_BOARD_HEX 4"
fi

if [[ $include_softdevice ]]; then
	ADD_SOFTDEVICE="${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_HEX}/${SOFTDEVICE}_softdevice.hex -intel"
fi
if [[ $include_firmware ]]; then
	ADD_BINARY="crownstone.bin -binary -offset $APPLICATION_START_ADDRESS"
	BINARY_VALID_FLAG="-exclude 0x7F000 0x7F004 -generate 0x7F000 0x7F004 -le-constant 0x00000001 4"
fi

cs_log "srec_cat $ADD_SOFTDEVICE $ADD_BOOTLOADER $HARDWARE_BOARD_CONFIG $ADD_BINARY $BINARY_VALID_FLAG $BOOTLOADER_SETTINGS -o combined.hex -intel"
srec_cat $ADD_SOFTDEVICE $ADD_BOOTLOADER $HARDWARE_BOARD_CONFIG $ADD_BINARY $BINARY_VALID_FLAG $BOOTLOADER_SETTINGS -o combined.hex -intel
res=$?
if [ ! $res -eq 0 ]; then
	cs_log "ERROR: srec_cat failed, check the results"
	exit 1
fi

cs_log "Combined binary is: ${BLUENET_BIN_DIR}/combined.hex"
