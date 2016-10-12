#!/bin/bash
#
# Parameter is optional, provide target if wanted. If left out, default target is crownstone

target=$1

source _utils.sh

###################
### Softdevice
###################

info "Build and upload softdevice ..."

./softdevice.sh all $target

checkError
succ "Softdevice DONE"

###################
### Firmware
###################

info "Build firmware ..."

./firmware.sh build $target

checkError
succ "Build DONE"

info "Upload firmware ..."

./firmware.sh upload $target

checkError
succ "Upload DONE"

###################
### Bootloader
###################

if [ -d "${BLUENET_BOOTLOADER_DIR}" ]; then
	info "Build bootloader ..."
	${BLUENET_BOOTLOADER_DIR}/scripts/all.sh $target

	checkError
	succ "Build DONE"
else
	err "BLUENET_BOOTLOADER_DIR not defined, skip bootloader!"
fi

info "Upload bootloader"

./firmware.sh bootloader $target

checkError

succ "Success!!"
