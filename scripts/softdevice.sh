#!/bin/bash

cmd=${1:? "$0 requires \"cmd\" as first argument"}

# get working path in absolute sense
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
working_path=$path

if [ ! -d "${BLUENET_DIR}" ]; then
	echo "BLUENET_DIR does not exist. Use .. as default"
	BLUENET_DIR=..
fi

#SD_BINDIR=${path}/../build/softdevice
SD_BINDIR=${BLUENET_DIR}/build

source config.sh

build() {
	echo "There is no real building step. Nordic provides a binary blob as SoftDevice"
	echo "However, we still need to extract the binary and the config blob"
	echo "  from $SOFTDEVICE_DIR/$SOFTDEVICE"
	./softdevice_objcopy.sh $SOFTDEVICE_DIR $SD_BINDIR $SOFTDEVICE $COMPILER_PATH $COMPILER_TYPE $SOFTDEVICE_NO_SEPARATE_UICR_SECTION
}

upload() {
	./softdevice_upload.sh $SD_BINDIR
}

all() {
	build
	sleep 1
	upload
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
		echo $"Usage: $0 {build|upload|switch|all}"
		exit 1
esac
