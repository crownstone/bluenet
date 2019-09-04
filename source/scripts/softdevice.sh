#!/bin/bash

usage() {
	echo "Usage: $0 {build|upload|clean|all} [target]"
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

SD_BINDIR=${BLUENET_BIN_DIR}



build() {
	cs_log "There is no real building step. Nordic provides a binary blob as SoftDevice"
	cs_log "However, we still need to extract the binary and the config blob from $SOFTDEVICE_DIR/$SOFTDEVICE_DIR_HEX"
	$path/_softdevice_objcopy.sh $SOFTDEVICE_DIR/$SOFTDEVICE_DIR_HEX $SD_BINDIR $SOFTDEVICE $COMPILER_PATH $COMPILER_TYPE $SOFTDEVICE_NO_SEPARATE_UICR_SECTION
	checkError "Error with building softdevice"
}

upload() {
	$path/_upload.sh $SD_BINDIR/softdevice_mainpart.hex $serial_num
	checkError "Failed to upload softdevice"
	$path/_writebyte.sh 0x10001018 0x7E000
	checkError "Failed to write MBR page address"
}

clean() {
	cs_log "Nothing to clean for softdevice"
}

all() {
	build
	upload
}

case "$cmd" in
	build)
		build
		;;
	upload)
		upload
		;;
	clean)
		clean
		;;
	all)
		all
		;;
	*)
		usage
		exit $CS_ERR_ARGUMENTS
esac
