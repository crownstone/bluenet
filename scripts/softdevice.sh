#!/bin/sh

cmd=${1:? "$0 requires \"cmd\" as first argument"}

# get working path in absolute sense
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
working_path=$path

echo "Load SoftDevice directory from configuration file"
source ../CMakeBuild.config

build() {
	echo "There is no real building step. Nordic provides a binary blob as SoftDevice"
	echo "However, we still need to extract the binary and the config blob"
	./softdevice_objcopy.sh $SOFTDEVICE_DIR $SOFTDEVICE $COMPILER_PATH $COMPILER_TYPE
}

upload() {
	./softdevice_upload.sh $SOFTDEVICE_DIR
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
		echo $"Usage: $0 {build|upload|all}"
		exit 1
esac

