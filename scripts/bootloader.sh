#!/bin/bash

usage() {
	echo "Usage: $0 {build|upload|debug|clean} [target [address|gdb_port]]"
}

cmd=${1:-help}
if [[ $cmd == "help" ]]; then
	usage
	exit 0
fi

# optional target, use crownstone as default
target=${2:-crownstone}

# optional address, use APPLICATION_START_ADDRESS as default
address=${3:-$APPLICATION_START_ADDRESS}

gdb_port=$3

# Get the scripts path: the path where this file is located.
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

cs_info "Load configuration from: ${path}/_check_targets.sh $target"
source ${path}/_check_targets.sh $target

cs_info "Load configuration from: ${path}/_config.sh"
source $path/_config.sh


if [ "$VERBOSE" == "1" ]; then
	cs_info "Verbose mode"
	make_flag=""
else
	cs_info "Silent mode"
	make_flag="-s"
fi

# use $APPLICATION_START_ADDRESS as default if no address defined
address=${address:-$APPLICATION_START_ADDRESS}



build() {
	if [ -d "${BLUENET_BOOTLOADER_DIR}" ]; then
		cs_info "Build bootloader ..."
		${BLUENET_BOOTLOADER_DIR}/scripts/all.sh $target
		checkError "Build failed"
		cs_succ "Build DONE"
	else
		cs_err "BLUENET_BOOTLOADER_DIR not defined."
		exit $CS_ERR_CONFIG
	fi
}

release() {
	build
}

upload() {
	${path}/_upload.sh $BLUENET_BIN_DIR/bootloader.hex $BOOTLOADER_START_ADDRESS $serial_num
	checkError "Uploading failed"
}

debug() {
	${path}/_debug.sh $BLUENET_BIN_DIR/bootloader.elf $serial_num $gdb_port
	checkError "Debugging failed"
}

clean() {
	if [ -d "${BLUENET_BOOTLOADER_DIR}" ]; then
		${BLUENET_BOOTLOADER_DIR}/scripts/clean.sh $target
		checkError "Cleanup failed"
	else
		cs_err "BLUENET_BOOTLOADER_DIR not defined."
		exit $CS_ERR_CONFIG
	fi
}

case "$cmd" in
	build)
		build
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

