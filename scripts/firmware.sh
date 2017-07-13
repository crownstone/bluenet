#!/bin/bash

getopt --test > /dev/null
if [[ $? -ne 4 ]]; then
  echo "I’m sorry, `getopt --test` failed in this environment."
  exit 1
fi

SHORT=c:t:a:
LONG=command:,target:,address:

PARSED=`getopt --options $SHORT --longoptions $LONG --name "$0" -- "$@"`
if [[ $? -ne 0 ]]; then
  exit 2
fi

eval set -- "$PARSED"

while true; do
	case "$1" in
		-c|--command)
			cmd=$2
			shift 2
			;;
		-t|--target) 
			target=$2
			shift 2
			;;
		-a|--address)
			address=$2
			shift 2
			;;
		--)
			shift
			break
			;;
		*)
			echo "Error in arguments."
			echo $2
			exit 3
			;;
	esac
done

usage() {
  echo "Bluenet firmware bash script (Feb. 2017)"
  echo
  echo "usage: ./firmware.sh [arguments]            run script for bluenet-compatible firmware"
  echo "   or: VERBOSE=1 ./firmware.sh [arguments]  run script in verbose mode"
  echo 
  echo "Required arguments:"
  echo "   -c command, --command=command            command to choose from this list:"
  echo "      build                                 cross-compile target"
  echo "      unit-test-host                        compile test units for host"
  echo "      unit-test-nrf5                        compile test units for nrf5 target"
  echo "      release                               create a release"
  echo "      upload                                upload application firmware to target"
  echo "      debug                                 debug application on target with gdb"
  echo "      all                                   build, upload, and debug"
  echo "      run                                   run application that is already uploaded"
  echo "      clean                                 clean compilation folders"
  echo "      bootloader-only                       upload bootloader"
  echo "      bootloader                            upload bootloader (what's the diff?)"
  echo "      debugbl                               debug bootloader"
  echo "      writeHardwareVersion                  write hardware version to target"
  echo 
  echo "Optional arguments:"
  echo "   -t target, --target=target               specify target (files are generated in separate directories)"
  echo "   -a address, --address address            specify particular address to use"
}

if [ ! "$cmd" ]; then
  usage
  exit 4
fi

# Default target
target=${target:-crownstone}

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

if [ "$VERBOSE" == "1" ]; then
  cs_info "Verbose mode"
  make_flag=""
else
  cs_info "Silent mode"
  make_flag="-s"
fi

# use $APPLICATION_START_ADDRESS as default if no address defined
address=${address:-$APPLICATION_START_ADDRESS}

git-pull() {
	cs_info "Pull from github"
	cd ${path}/.. && git pull
}

printf "${blue}\n"
printf "oo  _|_|_|    _|                                            _|     \n"
printf "oo  _|    _|  _|  _|    _|    _|_|    _|_|_|      _|_|    _|_|_|_| \n"
printf "oo  _|_|_|    _|  _|    _|  _|_|_|_|  _|    _|  _|_|_|_|    _|     \n"
printf "oo  _|    _|  _|  _|    _|  _|        _|    _|  _|          _|     \n"
printf "oo  _|_|_|    _|    _|_|_|    _|_|_|  _|    _|    _|_|_|      _|_| \n"
printf "${normal}\n"

cs_info "Configuration parameters loaded from files set up beforehand through e.g. _config.sh:"
printf "\n"
log_config

# Use hidden .build file to store variables
BUILD_PROCESS_FILE="$BLUENET_BUILD_DIR/.build"

if [ -e "$BLUENET_BUILD_DIR" ]; then
  if ! [ -e "$BUILD_PROCESS_FILE" ]; then
    BUILD_CYCLE=0
    echo "BUILD_CYCLE=$BUILD_CYCLE" > "$BUILD_PROCESS_FILE"
  fi
  source "$BUILD_PROCESS_FILE"

  BUILD_CYCLE=$((BUILD_CYCLE + 1))
  sed -i "s/\(BUILD_CYCLE *= *\).*/\1$BUILD_CYCLE/" "$BUILD_PROCESS_FILE"
  if ! (($BUILD_CYCLE % 100)); then
    printf "\n"
    printf "Would you like to check for updates? [Y/n]: "
    read update_response
    if [ "$update_response" == "n" ]; then
      git_version=$(git rev-parse --short=25 HEAD)
      printf "oo Git version: $git_version\n"
    else
      git-pull
    fi
  fi
fi

printf "${normal}\n"

unit-test-host() {
	cd ${path}/..
	cs_info "Execute make host-compile-target"
	make ${make_flag} host-compile-target
	# result=$?
	checkError "oo Error building firmware"
	cd $path
	# return $result
}

# todo: add more code to check if target exists
build() {
	cd ${path}/..
	cs_info "Execute make cross-compile-target"
	make ${make_flag} cross-compile-target
	# result=$?
	checkError "oo Error building firmware"
	cd $path
	# return $result
}

writeHardwareVersion() {
	verifyHardwareBoardDefined
	if [ $? -eq 0 ]; then
		# cs_info "HARDWARE_BOARD=$HARDWARE_BOARD"
		HARDWARE_BOARD_INT=`cat $BLUENET_DIR/include/cfg/cs_Boards.h | grep -o "#define.*\b$HARDWARE_BOARD\b.*" | grep -w "$HARDWARE_BOARD" | awk 'NF>1{print $NF}'`
		if [ $? -eq 0 ] && [ -n "$HARDWARE_BOARD_INT" ]; then
				cs_info "HARDWARE_BOARD $HARDWARE_BOARD = $HARDWARE_BOARD_INT"
				${path}/_writebyte.sh $HARDWARE_BOARD_ADDRESS `printf "%x" $HARDWARE_BOARD_INT` $serial_num
				checkError "Error writing hardware version"
		else
			cs_err "Failed to extract HARDWARE_BOARD=$HARDWARE_BOARD from $BLUENET_DIR/include/cfg/cs_Boards.h"
		fi
	fi
}

upload() {
	# verifyHardwareBoardDefined

	if [ $? -eq 0 ]; then
		# writeHardwareVersion
		${path}/_upload.sh $BLUENET_BIN_DIR/$target.hex $address $serial_num
		checkError "Error with uploading firmware"
	fi
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
	make ${make_flag} clean
	checkError "Error cleaning up"
}

uploadBootloader() {
	verifyHardwareBoardDefined

	if [ $? -eq 0 ]; then
		# perhaps do this separate anyway
		# ${path}/softdevice.sh all

		# note that within the bootloader the JLINK doesn't work anymore...
		# so perhaps first flash the binary and then the bootloader
		${path}/_upload.sh $BLUENET_BIN_DIR/bootloader.hex $BOOTLOADER_START_ADDRESS $serial_num

		checkError "Error uploading bootloader"

		# [26.01.17] uicr is cleared during bootloader upload, maybe because the bootloader needs
		#  to store some values into the uicr as well, so write the hardware version again after
		#  uploading the bootloader
		writeHardwareVersion

		# DE [12.10.16] is this still necessary? the bootloader is started automatically after
		#   uploading, and the app is started after marking as valid
	# 	if [ $? -eq 0 ]; then
	# 		sleep 1
	# 		# and set to load it
	# #		${path}/_writebyte.sh 0x10001014 $BOOTLOADER_REGION_START
	# 		${path}/_writebyte.sh 0x10001014 $BOOTLOADER_START_ADDRESS
	# 	fi
	fi
}

bootloader() {
	uploadBootloader
	if [ $? -eq 0 ]; then
		# Mark current app as valid app
		${path}/_writebyte.sh 0x0007F000 1 $serial_num

		checkError "Error marking app valid"
	fi
}

bootloader-only() {
	uploadBootloader
	if [ $? -eq 0 ]; then
		# Mark current app as invalid app
		${path}/_writebyte.sh 0x0007F000 0 $serial_num

		checkError "Error marking app invalid"
	fi
}

release() {
	cd ${path}/..
	cs_info "Execute make release"
	make release
	checkError "Failed to build release"
	# result=$?
	cd $path
	# return $result
}

verifyHardwareBoardDefined() {
	if [ -z "$HARDWARE_BOARD" ]; then
		cs_err "Need to specify HARDWARE_BOARD either in $BLUENET_CONFIG_DIR/_targets.sh"
		cs_err "for a given target, or by calling the script as"
		cs_err "   HARDWARE_BOARD=... ./firmware.sh"
		exit 1
	fi
}

case "$cmd" in
	build|unit-test-nrf5)
		build
		;;
	unit-test-host)
		unit-test-host
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
	writeHardwareVersion)
		writeHardwareVersion
		;;
	*)
		cs_info $"Usage: $0 {build|unit-test-host|unit-test-nrf5|upload|debug|all|run|clean|bootloader-only|bootloader|debugbl|release|writeHardwareVersion}"
		exit 1
esac

