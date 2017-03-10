#!/bin/bash

cmd=${1:? "$0 requires \"cmd\" as first argument"}

# optional target, use crownstone as default
target=${2:-crownstone}

# get working path in absolute sense
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

if [[ $cmd != "help" ]]; then

	# adjust targets and sets serial_num
	# call it with the . so that it get's the same arguments as the call to this script
	# and so that the variables assigned in the script will be persistent afterwards
	source ${path}/_check_targets.sh $target

	# configure environment variables, load configuration files
	source $path/_config.sh

	SD_BINDIR=${BLUENET_BIN_DIR}
fi

build() {
	log "There is no real building step. Nordic provides a binary blob as SoftDevice"
	log "However, we still need to extract the binary and the config blob"
	log "  from $SOFTDEVICE_DIR/$SOFTDEVICE_DIR_HEX"
	$path/_softdevice_objcopy.sh $SOFTDEVICE_DIR/$SOFTDEVICE_DIR_HEX $SD_BINDIR $SOFTDEVICE $COMPILER_PATH $COMPILER_TYPE $SOFTDEVICE_NO_SEPARATE_UICR_SECTION
	checkError "Error with building softdevice"
}

upload() {
	$path/_softdevice_upload.sh $SD_BINDIR $serial_num
	checkError "Error with uploading softdevice"
}

all() {
	build
	if [ $? -eq 0 ]; then
		sleep 1
		upload
	fi
}

case "$cmd" in
	build)
		build
		;;
	upload)
		upload
		;;
	all)
		all
		;;
	*)
		info "Usage: $0 {build|upload|all}"
		exit 1
esac
