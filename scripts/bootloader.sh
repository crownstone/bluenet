#!/bin/bash

usage() {
	echo "Usage: $0 {build|upload|debug|clean} [target [address [gdb_port]]]"
}

cmd=${1:-help}
if [[ $cmd == "help" ]]; then
	usage
	exit 0
fi

# optional target, use crownstone as default
target=${2:-crownstone}

# Get the scripts path: the path where this file is located.
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

cs_info "Load configuration from: ${path}/_check_targets.sh $target"
source ${path}/_check_targets.sh $target

cs_info "Load configuration from: ${path}/_config.sh"
source $path/_config.sh

# optional address, use BOOTLOADER_START_ADDRESS as default
address=${3:-$BOOTLOADER_START_ADDRESS}

gdb_port=${4:-$gdb_port}

if [ "$VERBOSE" == "1" ]; then
	cs_info "Verbose mode"
	make_flag=""
else
	cs_info "Silent mode"
	make_flag="-s"
fi



build() {
	cd ${path}/..
	cs_info "Execute make bootloader-compile-target"
	make ${make_flag} bootloader-compile-target
	checkError "Build failed"
	cd $path
}

build-settings() {
	if [ ! -f $BLUENET_BIN_DIR/crownstone.hex ]; then
		cs_err "File not found: $BLUENET_BIN_DIR/crownstone.hex"
		cs_err "Build the firmware first"
		exit $CS_ERR_FILE_NOT_FOUND
	fi
	cs_info "Generate bootloader settings"
	cs_warn "Assuming application-version 1 and bootloader-version 1 for now."
	nrfutil settings generate --family NRF52 --application "${BLUENET_BIN_DIR}/crownstone.hex" --application-version 1 --bootloader-version 1 --bl-settings-version 1 "${BLUENET_BIN_DIR}/bootloader-settings.hex"
	checkError "Generating bootloader settings. Do you have 'nrfutil' installed?"
}

release() {
	build
}

upload() {
	${path}/_upload.sh $BLUENET_BIN_DIR/bootloader.hex $address $serial_num
	checkError "Uploading failed"
}

upload-settings() {
	${path}/_upload.sh $BLUENET_BIN_DIR/bootloader-settings.hex $address $serial_num
	checkError "Uploading failed"
}

debug() {
	${path}/_debug.sh $BLUENET_BIN_DIR/bootloader.elf $serial_num $gdb_port
	checkError "Debugging failed"
}

clean() {
	cd ${path}/..
	make ${make_flag} clean
	checkError "Cleanup failed"
}

case "$cmd" in
	build)
		build
		;;
	build-settings)
		build-settings
		;;
	release)
		release
		;;
	upload)
		upload
		;;
	debug)
		debug
		;;
	clean)
		clean
		;;
	*)
		usage
		exit $CS_ERR_ARGUMENTS
esac

