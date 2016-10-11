#!/bin/bash

cmd=${1:? "Usage: $0 \"cmd\", \"target\""}

# use the current path as the bluenet directory
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export BLUENET_DIR=$(readlink -m ${path}/..)

if [[ $cmd != "help" ]]; then

	# adjust targets and sets serial_num
	# call it with the . so that it get's the same arguments as the call to this script
	# and so that the variables assigned in the script will be persistent afterwards
	. ${path}/_check_targets.sh

	# configure environment variables, load configuration files, check targets and
	# assign serial_num from target
	source $path/_config.sh
fi

# optional address, use $APPLICATION_START_ADDRESS as default
address=${3:-$APPLICATION_START_ADDRESS}

# todo: add more code to check if target exists
build() {
	cd ${path}/..
	make all
	result=$?
	cd $path
	return $result
}

upload() {
	${path}/_upload.sh $BLUENET_BIN_DIR/$target.hex $address $serial_num
	if [ $? -ne 0 ]; then
		echo "Error with uploading"
		exit 1
	fi
}

debug() {
	${path}/_debug.sh $BLUENET_BIN_DIR/$target.elf $serial_num $gdb_port
}

debugbl() {
	${path}/_debug.sh $BLUENET_BIN_DIR/bootloader.elf $serial_num $gdb_port
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
	${path}/_upload.sh $BLUENET_BIN_DIR/bootloader.hex $BOOTLOADER_START_ADDRESS $serial_num

	# Mark current app as valid app
	${path}/_writebyte.sh 0x0007F000 1

	if [ $? -eq 0 ]; then
		sleep 1
		# and set to load it
#		${path}/_writebyte.sh 0x10001014 $BOOTLOADER_REGION_START
		${path}/_writebyte.sh 0x10001014 $BOOTLOADER_START_ADDRESS
	fi
}

bootloader-only() {
	# perhaps do this separate anyway
	# ${path}/softdevice.sh all

	# note that within the bootloader the JLINK doesn't work anymore...
	# so perhaps first flash the binary and then the bootloader
	${path}/_upload.sh $BLUENET_BIN_DIR/bootloader.hex $BOOTLOADER_START_ADDRESS $serial_num

	# Mark current app as valid app
	${path}/_writebyte.sh 0x0007F000 0

	if [ $? -eq 0 ]; then
		sleep 1
		# and set to load it
#		${path}/_writebyte.sh 0x10001014 $BOOTLOADER_REGION_START
		${path}/_writebyte.sh 0x10001014 $BOOTLOADER_START_ADDRESS
	fi
}
release() {
	cd ${path}/..
	make release
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
	bootloader-only)
		bootloader-only
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
		echo $"Usage: $0 {build|upload|debug|all|run|clean|bootloader-only|bootloader|debugbl|release}"
		exit 1
esac

