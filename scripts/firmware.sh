#!/bin/bash

cmd=${1:? "Usage: $0 \"cmd\", \"target\""}

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

BLUENET_BUILD_DIR=build

if [[ $cmd != "help" ]]; then
	# adjust targets and sets serial_num
	# call it with the . so that it get's the same arguments as the call to this script
	# and so that the variables assigned in the script will be persistent afterwards
	. ${path}/_check_targets.sh
fi

source $path/_config.sh

# optional address, use $APPLICATION_START_ADDRESS as default
address=${3:-$APPLICATION_START_ADDRESS}

# todo: add more code to check if target exists
build() {
	cd ${path}/..
	make all BUILD_DIR=$BLUENET_BUILD_DIR
	result=$?
	cd $path
	return $result
}

upload() {
	${path}/_upload.sh $BLUENET_CONFIG_DIR/build/$target.hex $address $serial_num
	if [ $? -eq 0 ]; then
		echo "Error with uploading"
		exit 1
	fi
}

debug() {
	${path}/_debug.sh $BLUENET_CONFIG_DIR/build/$target.elf $serial_num $gdb_port
}

debugbl() {
	${path}/_debug.sh $BLUENET_CONFIG_DIR/build/bootloader.elf $serial_num $gdb_port
}

all() {
	build
	if [ $? -eq 0 ]; then
		sleep 1
		upload
		sleep 1
		debug
	fi
}

run() {
	build
	if [ $? -eq 0 ]; then
		sleep 1
		upload
	fi
}

clean() {
	cd ${path}/..
	make clean BUILD_DIR=$BLUENET_BUILD_DIR
}

bootloader() {
	# perhaps do this separate anyway
	# ${path}/softdevice.sh all

	# note that within the bootloader the JLINK doesn't work anymore...
	# so perhaps first flash the binary and then the bootloader
	${path}/_upload.sh $BLUENET_CONFIG_DIR/build/bootloader.hex $BOOTLOADER_START_ADDRESS $serial_num

	if [ $? -eq 0 ]; then
		sleep 1
		# and set to load it
		${path}/_writebyte.sh 0x10001014 $BOOTLOADER_REGION_START
	fi
}

release() {
	cd ${path}/..
	make release BUILD_DIR=$BLUENET_BUILD_DIR
	result=$?
	cd $path
	return $result
}

case "$cmd" in
	build)
		build
		;;
	upload)
		upload
		;;
	debug)
		debug
		;;
	all)
		all
		;;
	run)
		run
		;;
	clean)
		clean
		;;
	bootloader)
		bootloader
		;;
	debugbl)
		debugbl
		;;
	release)
		release
		;;
	*)
		echo $"Usage: $0 {build|upload|debug|clean|run|all}"
		exit 1
esac

