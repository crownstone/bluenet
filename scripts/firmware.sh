#!/bin/bash

cmd=${1:? "Usage: $0 \"cmd\", \"target\""}

if [[ $cmd != "help" && $cmd != "bootloader" ]]; then
	# target=${2:? "Usage: $0 \"cmd\", \"target\""}
	target=$2

	BLUENET_BUILD_DIR=build
	if [[ $target != "bootloader" && $target != "crownstone" ]]; then
		BLUENET_CONFIG_DIR=$BLUENET_CONFIG_DIR${target:+/$target}
		BLUENET_BUILD_DIR=$BLUENET_BUILD_DIR${target:+/$target}
		# BLUENET_CONFIG_DIR=$BLUENET_CONFIG_DIR/$target
		
		case "$target" in
			sirius)
				serial_num=480110849
				;;
			capella)
				serial_num=480207700
				;;
		esac
		target=crownstone
	fi
fi

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/config.sh

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
	${path}/upload.sh $BLUENET_CONFIG_DIR/build/$target.hex $address $serial_num
	if [ $? -eq 0 ]; then
		echo "Error with uploading"
		exit 1
	fi
	#${path}/upload.sh $BLUENET_CONFIG_DIR/build/$target.bin $address
}

debug() {
	${path}/debug.sh $BLUENET_CONFIG_DIR/build/$target.elf $serial_num
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
	make clean
}

bootloader() {
	# perhaps do this separate anyway
	# ${path}/softdevice.sh all

	# note that within the bootloader the JLINK doesn't work anymore...
	# so perhaps first flash the binary and then the bootloader
	${path}/firmware.sh upload bootloader 0x00034000
	
	if [ $? -eq 0 ]; then
		sleep 1
		# and set to load it
		${path}/writebyte.sh 0x10001014 0x00034000
	fi
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
	*)
		echo $"Usage: $0 {build|upload|debug|clean|run|all}"
		exit 1
esac

