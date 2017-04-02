#!/bin/bash
#
# Parameter is optional, provide target if wanted. If left out, default target is crownstone

target=$1
targetoption="-t $1"

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

###################
### Softdevice
###################

cs_info "Build and upload softdevice ..."

$path/softdevice.sh all $target

checkError
cs_succ "Softdevice DONE"

###################
### Firmware
###################

cs_info "Build firmware ..."

$path/firmware.sh -c build $targetoption

checkError
cs_succ "Build DONE"

cs_info "Upload firmware ..."

$path/firmware.sh -c upload $targetoption

checkError
cs_succ "Upload DONE"

###################
### Bootloader
###################

if [ -d "${BLUENET_BOOTLOADER_DIR}" ]; then
	cs_info "Build bootloader ..."
	${BLUENET_BOOTLOADER_DIR}/scripts/all.sh $target

	checkError
	cs_succ "Build DONE"
else
	cs_err "BLUENET_BOOTLOADER_DIR not defined, skip bootloader!"
fi

cs_info "Upload bootloader"

$path/firmware.sh -c bootloader $targetoption

checkError

cs_succ "Success!!"
