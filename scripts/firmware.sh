#!/bin/bash

cmd=${1:? "Usage: $0 \"cmd\", \"target\""}

if [[ $cmd != "help" && $cmd != "bootloader" ]]; then
	target=${2:? "Usage: $0 \"cmd\", \"target\""}
fi

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/config.sh

# optional address
address=$3

# todo: add more code to check if target exists
build() {
	cd ${path}/..
	make all
	result=$?
	cd $path
	return $result
}

upload() {
	${path}/upload.sh $BLUENET_DIR/build/$target.bin $address
}

debug() {
	${path}/debug.sh $BLUENET_DIR/$target.elf
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
	${path}/softdevice.sh all
	${path}/writebyte.sh 0x10001014 0x00034000
	${path}/firmware.sh all bootloader 0x00034000
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

