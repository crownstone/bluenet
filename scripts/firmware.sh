#!/bin/bash

cmd=${1:? "Usage: $0 \"cmd\", \"target\""}

# optional target, use crownstone as default
target=${2:-crownstone}

# optional address
address=$3

# use the current path as the bluenet directory
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

export BLUENET_DIR=$(readlink -m ${path}/..)

if [[ $cmd != "help" ]]; then

	# adjust targets and sets serial_num
	# call it with the . so that it get's the same arguments as the call to this script
	# and so that the variables assigned in the script will be persistent afterwards
	source ${path}/_check_targets.sh $target

	# configure environment variables, load configuration files, check targets and
	# assign serial_num from target
	source $path/_config.sh
fi

# use $APPLICATION_START_ADDRESS as default if no address defined
${address:=$APPLICATION_START_ADDRESS}

git-pull() {
	printf "oo Pull from github\n"
	cd ${path}/.. && git pull
}

printf "${blue}\n"
printf "oo  _|_|_|    _|                                            _|     \n"
printf "oo  _|    _|  _|  _|    _|    _|_|    _|_|_|      _|_|    _|_|_|_| \n"
printf "oo  _|_|_|    _|  _|    _|  _|_|_|_|  _|    _|  _|_|_|_|    _|     \n"
printf "oo  _|    _|  _|  _|    _|  _|        _|    _|  _|          _|     \n"
printf "oo  _|_|_|    _|    _|_|_|    _|_|_|  _|    _|    _|_|_|      _|_| \n"

# Use hidden .build file to store variables 
BUILD_PROCESS_FILE="$BLUENET_BUILD_DIR/.build"

if ! [ -e "$BUILD_PROCESS_FILE" ]; then
	BUILD_CYCLE=0
	echo "BUILD_CYCLE=$BUILD_CYCLE" >> "$BUILD_PROCESS_FILE"
fi
	
source "$BUILD_PROCESS_FILE"
BUILD_CYCLE=$((BUILD_CYCLE + 1))
sed -i "s/\(BUILD_CYCLE *= *\).*/\1$BUILD_CYCLE/" "$BUILD_PROCESS_FILE"
if ! (($BUILD_CYCLE % 100)); then
	printf "\n"
	printf "oo Would you like to check for updates? [Y/n]: " 
	read update_response
	if [ "$update_response" == "n" ]; then
		git_version=$(git rev-parse --short=25 HEAD)
		printf "oo Git version: $git_version\n"
	else
		git-pull
	fi
fi
printf "${normal}\n"
                                                                 
# todo: add more code to check if target exists
build() {
	cd ${path}/..
	info "oo Execute make (which will execute cmake)"
	make -s all
	# result=$?
	checkError "oo Error building firmware"
	cd $path
	# return $result
}

upload() {
	${path}/_upload.sh $BLUENET_BIN_DIR/$target.hex $address $serial_num
	checkError "Error with uploading firmware"
}

debug() {
	${path}/_debug.sh $BLUENET_BIN_DIR/$target.elf $serial_num $gdb_port
	checkError "Error debugging firmware"
}

debugbl() {
	${path}/_debug.sh $BLUENET_BIN_DIR/bootloader.elf $serial_num $gdb_port
	checkError "Error debugging bootloader"
}

all() {
	build
	if [ $? -eq 0 ]; then
		sleep 1
		upload
		if [ $? -eq 0 ]; then
			sleep 1
			debug
		fi
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
	make -s clean
	checkError "Error cleaning up"
}

bootloader() {
	# perhaps do this separate anyway
	# ${path}/softdevice.sh all

	# note that within the bootloader the JLINK doesn't work anymore...
	# so perhaps first flash the binary and then the bootloader
	${path}/_upload.sh $BLUENET_BIN_DIR/bootloader.hex $BOOTLOADER_START_ADDRESS $serial_num

	checkError "Error uploading bootloader"

	# Mark current app as valid app
	${path}/_writebyte.sh 0x0007F000 1

	checkError "Error marking app valid"

	# DE [12.10.16] is this still necessary? the bootloader is started automatically after
	#   uploading, and the app is started after marking as valid
# 	if [ $? -eq 0 ]; then
# 		sleep 1
# 		# and set to load it
# #		${path}/_writebyte.sh 0x10001014 $BOOTLOADER_REGION_START
# 		${path}/_writebyte.sh 0x10001014 $BOOTLOADER_START_ADDRESS
# 	fi
}

bootloader-only() {
	# perhaps do this separate anyway
	# ${path}/softdevice.sh all

	# note that within the bootloader the JLINK doesn't work anymore...
	# so perhaps first flash the binary and then the bootloader
	${path}/_upload.sh $BLUENET_BIN_DIR/bootloader.hex $BOOTLOADER_START_ADDRESS $serial_num

	checkError "Error uploading bootloader"

	# Mark current app as invalid app
	${path}/_writebyte.sh 0x0007F000 0

	checkError "Error marking app invalid"

	# DE [12.10.16] is this still necessary? the bootloader is started automatically after
	#   uploading
# 	if [ $? -eq 0 ]; then
# 		sleep 1
# 		# and set to load it
# #		${path}/_writebyte.sh 0x10001014 $BOOTLOADER_REGION_START
# 		${path}/_writebyte.sh 0x10001014 $BOOTLOADER_START_ADDRESS
# 	fi
}

release() {
	cd ${path}/..
	make release
	checkError "Failed to build release"
	# result=$?
	cd $path
	# return $result
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
		info $"Usage: $0 {build|upload|debug|all|run|clean|bootloader-only|bootloader|debugbl|release}"
		exit 1
esac

