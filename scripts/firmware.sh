#!/bin/bash

# Use "echo $?" to find out which error you have been able to generate :-)

ERR_GETOPT_TEST=101
ERR_GETOPT_PARSE=102
ERR_ARGUMENTS=103
ERR_UNKNOWN_COMMAND=104
ERR_VERIFY_HARDWARE_LOCALLY=105
ERR_HARDWARE_VERSION_UNSET=106
ERR_JLINK_NOT_FOUND=107
ERR_HARDWARE_VERSION_MISMATCH=108

getopt --test > /dev/null
if [[ $? -ne 4 ]]; then
  echo "I’m sorry, `getopt --test` failed in this environment."
  exit $ERR_GETOPT_TEST
fi

SHORT=c:t:a:ybsu
LONG=command:,target:,address:yes,bootloader,softdevice,use_combined

PARSED=$(getopt --options $SHORT --longoptions $LONG --name "$0" -- "$@")
if [[ $? -ne 0 ]]; then
  exit $ERR_GETOPT_PARSE
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
		-y|--yes)
			autoyes=true
			shift 1
			;;
		-b|--bootloader)
			include_bootloader=true
			shift 1
			;;
		-s|--softdevice)
			include_softdevice=true
			shift 1
			;;
		-u|--use_combined)
			use_combined=true
			shift 1
			;;
		--)
			shift
			break
			;;
		*)
			echo "Error in arguments."
			echo $2
			exit $ERR_ARGUMENTS
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
  echo "      bootloader                            upload bootloader and application"
  echo "      softdevice                            upload softdevice"
  echo "      debugbl                               debug bootloader"
  echo "      readHardwareVersion                   read hardware version from target"
  echo "      writeHardwareVersion                  write hardware version to target"
  echo 
  echo "Optional arguments:"
  echo "   -t target, --target=target               specify target (files are generated in separate directories)"
  echo "   -a address, --address address            specify particular address to use"
  echo "   -b, --bootloader                         include bootloader"
  echo "   -s, --softdevice                         include softdevice"
  echo "   -u, --use_combined                       run combine script and use combined binary"
  echo "   -y, --yes                                automatically respond yes (non-interactive mode)"
}

if [ ! "$cmd" ]; then
  usage
  exit $ERR_UNKNOWN_COMMAND
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
	cs_info "Load configuration from: ${path}/_check_targets.sh $target"
	source ${path}/_check_targets.sh $target

	# configure environment variables, load configuration files, check targets and
	# assign serial_num from target
	cs_info "Load configuration from: ${path}/_config.sh"
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

# Use hidden .build file to store variables (it is just used to increment a number and ask for a rebuild every 100x)
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
	verifyHardwareBoardDefinedLocally
	if [ $? -eq 0 ]; then
		HARDWARE_BOARD_INT=$(grep -oP "#define\s+$HARDWARE_BOARD\s+\d+" $BLUENET_DIR/include/cfg/cs_Boards.h | grep -oP "\d+$")
		if [ $? -eq 0 ] && [ -n "$HARDWARE_BOARD_INT" ]; then
			cs_info "HARDWARE_BOARD $HARDWARE_BOARD = $HARDWARE_BOARD_INT"
			HARDWARE_BOARD_HEX=$(printf "%x" $HARDWARE_BOARD_INT)
			${path}/_writebyte.sh $HARDWARE_BOARD_ADDRESS $HARDWARE_BOARD_HEX $serial_num
			checkError "Error writing hardware version"
		else
			cs_err "Failed to extract HARDWARE_BOARD=$HARDWARE_BOARD from $BLUENET_DIR/include/cfg/cs_Boards.h"
		fi
	fi
}

upload() {
	if [ $use_combined ]; then
		cs_info "Upload all at once"
	else
		verifyHardwareBoardDefined
	fi
	if [ $? -eq 0 ]; then
	        if [ $include_bootloader ]; then
		      bootloader
		fi
	        if [ $include_softdevice ]; then
		      softdevice
		fi
		# Upload application firmware
		if [ $use_combined ]; then
		      combine
		      ${path}/_upload.sh $BLUENET_BIN_DIR/combined.hex $address $serial_num
		      checkError "Error with uploading firmware"
		else
		      ${path}/_upload.sh $BLUENET_BIN_DIR/$target.hex $address $serial_num
		      checkError "Error with uploading firmware"
		fi
	fi
}

combine() {
	./combine.sh $target
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

softdevice() {
	verifyHardwareBoardDefined

	if [ $? -eq 0 ]; then
		${path}/softdevice.sh all
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
		# Mark current app as an invalid app
		${path}/_writebyte.sh 0x0007F000 0 $serial_num

		checkError "Error marking app invalid, note INVALID (only a bootloader is flashed, no valid app)"
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

# The hardware board version should be defined in the file:
#   $BLUENET_CONFIG_DIR/$target/CMakeBuild.config
# If it is not defined it is impossible to check if we actually flash the right firmware so the script will exit.
verifyHardwareBoardDefinedLocally() {
	if [ -z "$HARDWARE_BOARD" ]; then
		cs_err 'Need to specify HARDWARE_BOARD through $BLUENET_CONFIG_DIR/_targets.sh'
		cs_err 'Which pulls in $BLUENET_CONFIG_DIR/$target/CMakeBuild.config'
		cs_err 'for a given target, or by calling the script as'
		cs_err '   HARDWARE_BOARD=... ./firmware.sh'
		exit $ERR_VERIFY_HARDWARE_LOCALLY
	fi 
}

# The hardware board is actually physically checked. 
verifyHardwareBoardDefined() {
	verifyHardwareBoardDefinedLocally
	cs_info "Find hardware board version via ${path}/_readbyte.sh $HARDWARE_BOARD_ADDRESS $serial_num"
	hardware_board_version=$(${path}/_readbyte.sh $HARDWARE_BOARD_ADDRESS $serial_num)
	HARDWARE_BOARD_INT=$(grep -oP "#define\s+$HARDWARE_BOARD\s+\d+" $BLUENET_DIR/include/cfg/cs_Boards.h | grep -oP "\d+$")
	config_board_version=$(printf "%08x" $HARDWARE_BOARD_INT)
	if [ "$hardware_board_version" == 'FFFFFFFF' ] || [ "$hardware_board_version" == 'ffffffff' ]; then
	  if [ $autoyes ]; then
	    cs_info "Automatically overwriting hardware version (now at 0xFFFFFFFF)"
	  else
	    cs_info "Hardware version is $hardware_board_version"
	    echo -n "Do you want to overwrite the hardware version [y/N]? "
	    read autoyes
	  fi
	  if [ "$autoyes" == 'true' ] || [ "$autoyes" == 'Y' ] || [ "$autoyes" == 'y' ]; then 
	    writeHardwareVersion
	  else 
	    cs_err "You have to write the hardware version! It is still set to $hardware_board_version."
	    exit $ERR_HARDWARE_VERSION_UNSET
	  fi
	elif [ "$hardware_board_version" == '<not found>' ]; then
	  cs_err "Did you actually connect the JLink?"
	  exit $ERR_JLINK_NOT_FOUND
	elif [ "$hardware_board_version" != "$config_board_version" ]; then
	  cs_err "You have an incorrect hardware version on your board! It is set to $hardware_board_version rather than $config_board_version."
	  exit $ERR_HARDWARE_VERSION_MISMATCH
	else
	  cs_info "Found hardware version: $hardware_board_version"
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
	readHardwareVersion)
		verifyHardwareBoardDefined
		;;
	writeHardwareVersion)
		writeHardwareVersion
		;;
	*)
		cs_info $"Usage: $0 {build|unit-test-host|unit-test-nrf5|upload|debug|all|run|clean|bootloader-only|bootloader|debugbl|release|writeHardwareVersion}"
		exit $ERR_ARGUMENTS
esac

