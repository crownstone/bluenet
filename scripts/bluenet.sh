#!/bin/bash

# Get the scripts path: the path where this file is located.
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

bluenet_logo

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
	echo
	echo "What to build and/or upload:"
	echo "   -F, --firmware                       include firmware"
	echo "   -B, --bootloader                     include bootloader"
	echo "   -S, --softdevice                     include softdevice"
	echo "   -H, --hardware_board_version         include hardware board version"
	echo "   -C, --combined                       build and/or upload combined binary, always includes firmware, bootloader, softdevice, and hardware version"
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
	exit $CS_ERR_ARGUMENTS
fi

getopt --test > /dev/null
if [[ $? -ne 4 ]]; then
	cs_err "Error: \"getopt --test\" failed in this environment. Please install the GNU version of getopt."
	exit $CS_ERR_GETOPT_TEST
fi

SHORT=t:a:beudcyhFBSHC
LONG=target:,address:build,erase,upload,debug,clean,yes,help,firmware,bootloader,softdevice,hardware_version,combined,unit_test_host,unit_test_nrf5

PARSED=$(getopt --options $SHORT --longoptions $LONG --name "$0" -- "$@")
if [[ $? -ne 0 ]]; then
	exit $CS_ERR_GETOPT_PARSE
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
		-H|--hardware_board_version)
			include_board_version=true
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
			exit 0
			shift 1
			;;

		--)
			shift
			break
			;;
		*)
			echo "Error in arguments."
			echo $2
			exit $CS_ERR_ARGUMENTS
			;;
	esac
done


# Default target
target=${target:-crownstone}

# Adjust config, build, bin, and release dirs. Assign serial_num and gdb_port from target.
cs_info "Load configuration from: ${path}/_check_targets.sh $target"
source ${path}/_check_targets.sh $target

# Check environment variables, load configuration files.
cs_info "Load configuration from: ${path}/_config.sh"
source $path/_config.sh

# Use $APPLICATION_START_ADDRESS as default if no address defined.
address=${address:-$APPLICATION_START_ADDRESS}

cs_info "Configuration parameters loaded from files set up beforehand through e.g. _config.sh:"
log_config
echo





build_firmware() {
	${path}/firmware.sh build $target
	checkError "Error building firmware"
}

build_firmware_release() {
	${path}/firmware.sh release $target
	checkError "Error building firmware release"
}

build_bootloader_settings() {
	${path}/bootloader.sh build-settings $target
	checkError "Error building bootloader settings"
}

build_bootloader() {
	${path}/bootloader.sh build $target
	checkError "Error building bootloader"
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
	${path}/firmware.sh unit-test-host $target
	checkError "Error building unit test host"
}


erase_flash() {
	${path}/_erase_flash.sh $serial_num
	checkError "Error erasing flash"
}


upload_firmware() {
	${path}/firmware.sh upload $target $address
	checkError "Error uploading firmware"
}

upload_bootloader_settings() {
	${path}/bootloader.sh upload-settings $target
	checkError "Error uploading bootloader settings"
}

upload_bootloader() {
	${path}/bootloader.sh upload $target
	checkError "Error uploading bootloader"
}

upload_softdevice() {
	${path}/softdevice.sh upload $target
	checkError "Error uploading softdevice"
}

upload_combined() {
	cs_info "Upload all at once"
	${path}/_upload.sh $BLUENET_BIN_DIR/combined.hex $serial_num
	checkError "Error uploading combined binary"
}

upload_board_version() {
	${path}/board_version.sh upload $target
	checkError "Error uploading board version"
}

debug_firmware() {
	${path}/firmware.sh debug $target
	checkError "Error debugging firmware"
}

debug_bootloader() {
	${path}/bootloader.sh debug $target
	checkError "Error debugging bootloader"
}




clean_firmware() {
	${path}/firmware.sh clean $target
	checkError "Error cleaning up firmware"
}

clean_bootloader() {
	${path}/bootloader.sh clean $target
	checkError "Error cleaning up bootloader"
}

clean_softdevice() {
	${path}/softdevice.sh clean $target
	checkError "Error cleaning up softdevice"
}



verify_board_version_written() {
	${path}/board_version.sh check $target
	checkError "Error: no correct board version is written."
}

reset() {
	${path}/reset.sh $serial_num
}


# Main
done_something=false

if [ $do_unit_test_nrf5 ]; then
	cs_info "Unit test NRF5: You can also just build firmware, as long as TEST_TARGET=\"nrf5\" is in the config."
	if [ "$TEST_TARGET" != "nrf5" ]; then
		cs_err "Target needs to have TEST_TARGET=\"nrf5\" in config."
		exit $CS_ERR_CONFIG
	fi
	do_build=true
	include_firmware=true
fi

if [ $do_build ]; then
	done_something=true
	done_built=false
	if [ $include_firmware ]; then
		if [ $release_build ]; then
			build_firmware_release
		else
			build_firmware
		fi
		build_bootloader_settings
		done_built=true
	fi
	if [ $include_bootloader ]; then
		build_bootloader_settings
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
		cs_err "Nothing was built. Please specify what to build."
		exit $CS_ERR_NOTHING_INCLUDED
	fi
fi


if [ $do_erase_flash ]; then
	done_something=true
	erase_flash
fi


if [ $do_upload ]; then
	done_something=true
	done_upload=false
	if [ $use_combined ]; then
		if [ $include_softdevice -o $include_bootloader -o $include_firmware -o $include_board_version ]; then
			# I guess it makes sense to rebuild the combined when one of the others was included?
			build_combined
		fi
		upload_combined
#		if [ $include_bootloader ]; then
#			${path}/_writebyte.sh 0x10001014 $BOOTLOADER_START_ADDRESS
#		fi
		done_upload=true
	else
		if [ $include_softdevice ]; then
			upload_softdevice
			done_upload=true
		fi
		if [ $include_bootloader ]; then
			upload_bootloader
			# If not including here as well, it is very annoying that consecutive builds do not including settings.
			# Consecutive calls to
			# ./bluenet -uF
			# ./bluenet -uB
			# should have same result as ./bluenet -uBF
			upload_bootloader_settings
			done_upload=true
		fi
		# Write board version before uploading firmware, else firmware writes it.
		if [ $include_board_version ]; then
			upload_board_version
			done_upload=true
		fi
		if [ $include_firmware ]; then
			upload_firmware
			if [ $include_bootloader ]; then
				upload_bootloader_settings
			fi
			done_upload=true
		else
			:
		fi
	fi
	verify_board_version_written
	if [ "$done_upload" != true ]; then
		cs_err "Nothing was uploaded. Please specify what to upload."
		exit $CS_ERR_NOTHING_INCLUDED
	fi
	reset
fi


if [ $do_debug ]; then
	done_something=true
	if [ $include_bootloader ]; then
		debug_bootloader
	elif [ $include_firmware ]; then
		debug_firmware
	else
		debug_firmware
	fi
fi


if [ $do_clean ]; then
	done_something=true
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
		cs_err "Nothing was cleaned. Please specify what to clean."
		exit $CS_ERR_NOTHING_INCLUDED
	fi
fi


if [ $do_unit_test_host ]; then
	done_something=true
	build_unit_test_host
fi


if [ "$done_something" != true ]; then
	cs_err "Nothing was done. Please specify a command."
	exit $CS_ERR_NO_COMMAND
fi
cs_succ "Done!"
