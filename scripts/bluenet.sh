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
ERR_CONFIG=109

usage() {
	echo "Bluenet bash script"
	echo
	echo "usage: ./bluenet.sh [args]              run script"
	echo "   or: VERBOSE=1 ./bluenet.sh [args]    run script in verbose mode"
	echo
	echo "Commands:"
	echo "   -b, --build                          cross-compile target"
	echo "   -e, --erase                          erase flash of target"
	echo "   -u, --upload                         upload binary to target"
	echo "   -d, --debug                          debug firmware"
	echo "   -c, --clean                          clean build dir"
	echo "   --unit_test_host                     compile unit tests for host"
	echo "   --unit_test_nrf5                     compile unit tests for nrf5"
#	echo "   --read_hardware_version              read hardware version from target"
	echo
	echo "What to build and/or upload:"
	echo "   -F, --firmware                       include firmware"
	echo "   -B, --bootloader                     include bootloader"
	echo "   -S, --softdevice                     include softdevice"
	echo "   -H, --hardware_verion                include hardware version"
	echo "   -C, --combined                       build and/or upload combined binary"
	echo
	echo "Extra arguments:"
	echo "   -t target, --target target           specify config target (files are generated in separate directories)"
	echo "   -a address, --address address        specify address on flash to upload to"
	echo "   -r, --release_build                  compile a release build"
	echo "   -y, --yes                            automatically select default option (non-interactive mode)"
	echo "   -h, --help                           show this help"
}

if [[ $# -lt 1 ]]; then
	usage
	exit $ERR_ARGUMENTS
fi

getopt --test > /dev/null
if [[ $? -ne 4 ]]; then
	echo "I’m sorry, `getopt --test` failed in this environment."
	exit $ERR_GETOPT_TEST
fi

SHORT=t:a:beudcyhFBSHC
LONG=target:,address:build,erase,upload,debug,clean,yes,help,firmware,bootloader,softdevice,hardware_version,combined

PARSED=$(getopt --options $SHORT --longoptions $LONG --name "$0" -- "$@")
if [[ $? -ne 0 ]]; then
	exit $ERR_GETOPT_PARSE
fi

eval set -- "$PARSED"


while true; do
	case "$1" in
		-b|--build)
			do_build=true
			shift 1
			;;
		-e|--erase)
			do_erase_flash=true
			shift 1
			;;
		-u|--upload)
			do_upload=true
			shift 1
			;;
		-d|--debug)
			do_debug=true
			shift 1
			;;
		-c|--clean)
			do_clean=true
			shift 1
			;;
		--unit_test_host)
			do_unit_test_host=true
			shift 1
			;;
		--unit_test_nrf5)
			do_unit_test_nrf5=true
			shift 1
			;;

		-F|--firmware)
			include_firmware=true
			shift 1
			;;
		-B|--bootloader)
			include_bootloader=true
			shift 1
			;;
		-S|--softdevice)
			include_softdevice=true
			shift 1
			;;
		-H|--hardware_version)
			include_hardware_version=true
			shift 1
			;;
		-C|--combined)
			use_combined=true
			shift 1
			;;

		-t|--target)
			target=$2
			shift 2
			;;
		-a|--address)
			address=$2
			shift 2
			;;
		-r|--release_build)
			release_build=true
			shift 1
			;;
		-y|--yes)
			autoyes=true
			shift 1
			;;
		-h|--help)
			usage
			exit
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


# Default target
target=${target:-crownstone}

# Use the current path as the bluenet directory
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

bluenet_logo

export BLUENET_DIR=$(readlink -m ${path}/..)

# adjust targets and sets serial_num
# call it with the . so that it get's the same arguments as the call to this script
# and so that the variables assigned in the script will be persistent afterwards
cs_info "Load configuration from: ${path}/_check_targets.sh $target"
source ${path}/_check_targets.sh $target

# configure environment variables, load configuration files, check targets and
# assign serial_num from target
cs_info "Load configuration from: ${path}/_config.sh"
source $path/_config.sh

# Set make flags
if [ "$VERBOSE" == "1" ]; then
	cs_info "Verbose mode"
	make_flag=""
else
	cs_info "Silent mode"
	make_flag="-s"
fi

# use $APPLICATION_START_ADDRESS as default if no address defined
address=${address:-$APPLICATION_START_ADDRESS}

cs_info "Configuration parameters loaded from files set up beforehand through e.g. _config.sh:"
log_config
echo





build_firmware() {
	cd ${path}/..
	cs_info "Execute make cross-compile-target"
	make ${make_flag} cross-compile-target
	checkError "Error building firmware"
	cd $path
}

build_firmware_release() {
	cd ${path}/..
	cs_info "Execute make release"
	make ${make_flag} release
	checkError "Failed to build release"
	cd $path
}

build_bootloader() {
	if [ -d "${BLUENET_BOOTLOADER_DIR}" ]; then
		cs_info "Build bootloader ..."
		${BLUENET_BOOTLOADER_DIR}/scripts/all.sh $target
		checkError "Error firmware softdevice"
		cs_succ "Build DONE"
	else
		cs_err "BLUENET_BOOTLOADER_DIR not defined."
		exit $ERR_CONFIG
	fi
}

build_softdevice() {
	${path}/softdevice.sh build $target
	checkError "Error building softdevice"
}

build_combined() {
	${path}/combine.sh -f -b -s -h -t $target
	checkError "Error combining to binary"
}

build_unit_test_host() {
	cd ${path}/..
	cs_info "Execute make host-compile-target"
	make ${make_flag} host-compile-target
	checkError "Error building unit test host"
	cd $path
}


erase_flash() {
	${path}/_erase_flash.sh $serial_num
	checkError "Error erasing flash"
}


upload_firmware() {
	${path}/_upload.sh $BLUENET_BIN_DIR/$target.hex $address $serial_num
	checkError "Error uploading firmware"
}

upload_bootloader() {
	${path}/_upload.sh $BLUENET_BIN_DIR/bootloader.hex $BOOTLOADER_START_ADDRESS $serial_num
	checkError "Error uploading bootloader"
}

upload_softdevice() {
	${path}/softdevice.sh upload $target
	checkError "Error uploading softdevice"
}

upload_combined() {
	cs_info "Upload all at once"
	${path}/_upload.sh $BLUENET_BIN_DIR/combined.hex $address $serial_num
	checkError "Error uploading combined binary"
}

upload_hardware_version() {
	verify_hardware_version_defined

	HARDWARE_BOARD_INT=$(grep -oP "#define\s+$HARDWARE_BOARD\s+\d+" $BLUENET_DIR/include/cfg/cs_Boards.h | grep -oP "\d+$")
	if [ $? -eq 0 ] && [ -n "$HARDWARE_BOARD_INT" ]; then
		cs_info "HARDWARE_BOARD $HARDWARE_BOARD = $HARDWARE_BOARD_INT"
		HARDWARE_BOARD_HEX=$(printf "%x" $HARDWARE_BOARD_INT)
		${path}/_writebyte.sh $HARDWARE_BOARD_ADDRESS $HARDWARE_BOARD_HEX $serial_num
		checkError "Error writing hardware version"
	else
		cs_err "Failed to extract HARDWARE_BOARD=$HARDWARE_BOARD from $BLUENET_DIR/include/cfg/cs_Boards.h"
		exit $ERR_CONFIG
	fi
}

debug_firmware() {
	${path}/_debug.sh $BLUENET_BIN_DIR/$target.elf $serial_num $gdb_port
	checkError "Error debugging firmware"
}

debug_bootloader() {
	${path}/_debug.sh $BLUENET_BIN_DIR/bootloader.elf $serial_num $gdb_port
	checkError "Error debugging bootloader"
}



upload_valid_app() {
	cs_info "Mark current app as valid"
	${path}/_writebyte.sh 0x0007F000 1 $serial_num
	checkError "Error marking app valid"
}

upload_invalid_app() {
	cs_info "Mark current app as invalid"
	${path}/_writebyte.sh 0x0007F000 0 $serial_num
	checkError "Error marking app invalid"
}




clean_firmware() {
	cd ${path}/..
	make ${make_flag} clean
	checkError "Error cleaning up"
}

clean_bootloader() {
	cs_warn "TODO: clean bootloader"
}

clean_softdevice() {
	${path}/softdevice.sh build $target
	checkError "Error building softdevice"
}




# The hardware board version should be defined in the file:
#   $BLUENET_CONFIG_DIR/$target/CMakeBuild.config
# If it is not defined it is impossible to check if we actually flash the right firmware so the script will exit.
verify_hardware_version_defined() {
	if [ -z "$HARDWARE_BOARD" ]; then
		cs_err 'Need to specify HARDWARE_BOARD through $BLUENET_CONFIG_DIR/_targets.sh'
		cs_err 'Which pulls in $BLUENET_CONFIG_DIR/$target/CMakeBuild.config'
		cs_err 'for a given target, or by calling the script as'
		cs_err '   HARDWARE_BOARD=... ./firmware.sh'
		exit $ERR_VERIFY_HARDWARE_LOCALLY
	fi
}

# The hardware board is actually physically checked.
verify_hardware_version_written() {
	verify_hardware_version_defined
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
			upload_hardware_version
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


# Main
if [ $do_build ]; then
	done_built=false
	if [ $include_firmware ]; then
		if [ $release_build ]; then
			build_firmware_release
		else
			build_firmware
		fi
		done_built=true
	fi
	if [ $include_bootloader ]; then
		build_bootloader
		done_built=true
	fi
	if [ $include_softdevice ]; then
		build_softdevice
		done_built=true
	fi
	if [ $use_combined ]; then
		build_combined
		done_built=true
	fi
	if [ "$done_built" != true ]; then
		cs_err "Nothing was built"
		exit 1
	fi
fi


if [ $do_erase_flash ]; then
	erase_flash
fi


if [ $do_upload ]; then
	done_upload=false
	if [ $use_combined ]; then
		upload_combined
		done_upload=true
	else
		if [ $include_softdevice ]; then
			upload_softdevice
			done_upload=true
		fi
		if [ $include_bootloader ]; then
			upload_bootloader
			done_upload=true
		fi
		if [ $include_firmware ]; then
			upload_firmware
			upload_valid_app
			done_upload=true
		else
			upload_invalid_app
		fi
		if [ $include_hardware_version ]; then
			upload_hardware_version
			done_upload=true
		fi
	fi
	if [ "$done_upload" != true ]; then
		cs_err "Nothing was uploaded"
		exit 1
	fi
fi


if [ $do_debug ]; then
	if [ $include_firmware ]; then
		debug_firmware
	elif [ $include_bootloader ]; then
		debug_bootloader
	else
		debug_firmware
	fi
fi


if [ $do_clean ]; then
	done_clean=false
	if [ $include_firmware ]; then
		clean_firmware
		done_clean=true
	fi
	if [ $include_bootloader ]; then
		clean_bootloader
		done_clean=true
	fi
	if [ $include_softdevice ]; then
		clean_softdevice
		done_clean=true
	fi
	if [ "$done_clean" != true ]; then
		cs_err "Nothing was cleaned"
		exit 1
	fi
fi


if [ $do_unit_test_host ]; then
	build_unit_test_host
fi


if [ $do_unit_test_nrf5 ]; then
	cs_warn "TODO: unit_test_nrf5"
fi

cs_succ "Done!"
