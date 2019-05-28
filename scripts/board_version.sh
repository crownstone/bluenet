#!/bin/bash

usage() {
	echo "Usage: $0 {read|check|upload} [target]"
}

cmd=${1:-help}
if [[ $cmd == "help" ]]; then
	usage
	exit 0
fi

# optional target, use crownstone as default
target=${2:-crownstone}

# Get the scripts path: the path where this file is located.
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

cs_info "Load configuration from: ${path}/_check_targets.sh $target"
source ${path}/_check_targets.sh $target

cs_info "Load configuration from: ${path}/_config.sh"
source $path/_config.sh


# The board version should be defined in the file: $BLUENET_CONFIG_DIR/$target/CMakeBuild.config
# If it is not defined it is impossible to check if we actually flash the right firmware so the script will exit.
verify_board_version_defined() {
	if [ -z "$HARDWARE_BOARD" ]; then
		cs_err 'Need to specify HARDWARE_BOARD through $BLUENET_CONFIG_DIR/_targets.sh'
		cs_err 'Which pulls in $BLUENET_CONFIG_DIR/$target/CMakeBuild.config'
		cs_err 'for a given target, or by calling the script as'
		cs_err '   HARDWARE_BOARD=... ./firmware.sh'
		exit $ERR_VERIFY_HARDWARE_LOCALLY
	fi
}

# Upload board version. Only works when no board version is written yet!
upload() {
	verify_board_version_defined
	HARDWARE_BOARD_INT=$(grep -oP "#define\s+$HARDWARE_BOARD\s+\d+" $BLUENET_DIR/include/cfg/cs_Boards.h | grep -oP "\d+$")
	if [ $? -eq 0 ] && [ -n "$HARDWARE_BOARD_INT" ]; then
		cs_info "HARDWARE_BOARD $HARDWARE_BOARD = $HARDWARE_BOARD_INT"
		HARDWARE_BOARD_HEX=$(printf "%x" $HARDWARE_BOARD_INT)
		$path/_writebyte.sh $HARDWARE_BOARD_ADDRESS "0x$HARDWARE_BOARD_HEX" $serial_num
		checkError "Error writing board version"
	else
		cs_err "Failed to extract HARDWARE_BOARD=$HARDWARE_BOARD from $BLUENET_DIR/include/cfg/cs_Boards.h"
		exit $ERR_CONFIG
	fi
}

read_board_version() {
	cs_log "Find board version via ${path}/_readbyte.sh $HARDWARE_BOARD_ADDRESS $serial_num"
	hardware_board_version=$(${path}/_readbyte.sh $HARDWARE_BOARD_ADDRESS $serial_num)
	checkError "Failed to read board version"
	echo "$hardware_board_version"
}

# Checks if a board version is already written.
# If so, and it's different from config, then it will give an error.
# If nothing is written, it will give an error.
check() {
	verify_board_version_defined
	read_board_version
	HARDWARE_BOARD_INT=$(grep -oP "#define\s+$HARDWARE_BOARD\s+\d+" $BLUENET_DIR/include/cfg/cs_Boards.h | grep -oP "\d+$")
	config_board_version=$(printf "0x%08x" $HARDWARE_BOARD_INT)
	if [ "$hardware_board_version" == '0xFFFFFFFF' ] || [ "$hardware_board_version" == '0xffffffff' ]; then
#		cs_log "No board version is written yet, writing the one from config."
#		upload
		cs_info "Board version is not written."
		if [ $autoyes ]; then
			cs_info "Automatically writing board version."
		else
			echo -n "Do you want to overwrite the board version [Y/n]? "
			read overwrite_board_version
			# default overwrite
			overwrite_board_version=${overwrite_board_version:-y}
		fi
		# Default yes
		if [ "$autoyes" == 'true' ] || [ "$overwrite_board_version" == 'Y' ] || [ "$overwrite_board_version" == 'y' ]; then 
			upload
		else 
			cs_err "You have to write the board version! It is still set to $hardware_board_version."
			exit $ERR_HARDWARE_VERSION_UNSET
		fi
	elif [ "$hardware_board_version" == '<not found>' ]; then
		cs_err "Did you actually connect the JLink?"
		exit $ERR_JLINK_NOT_FOUND
	elif [ "$hardware_board_version" != "$config_board_version" ]; then
		cs_err "You have an incorrect board version on your hardware! It is set to $hardware_board_version rather than $config_board_version."
		cs_err "This value cannot be overwritten, you have to erase first."
		exit $ERR_HARDWARE_VERSION_MISMATCH
#		if [ $autoyes ]; then
#			#cs_info "Automatically overwriting hardware version (now at $hardware_board_version)"
#			: #noop
#		else
#			cs_info "Hardware board version is $hardware_board_version"
#			echo -n "Do you want to overwrite the hardware version [y/N]? "
#			read overwrite_hardware_board_version
#		fi
#		# Default no
#		if [ "$overwrite_hardware_board_version" == 'Y' ] || [ "$overwrite_hardware_board_version" == 'y' ]; then
#			upload
#		else
#			cs_err "You have a different board version on your hardware! It is set to $hardware_board_version rather than $config_board_version."
#			exit $ERR_HARDWARE_VERSION_MISMATCH
#		fi
	else
		cs_info "Found board version: $hardware_board_version"
	fi
}

case "$cmd" in
	read)
		read_board_version
		;;
	check)
		check
		;;
	upload)
		upload
		;;
	*)
		usage
		exit $CS_ERR_ARGUMENTS
esac

