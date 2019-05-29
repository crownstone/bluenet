#!/bin/bash

###################################################################
# uses targets , e.g.
#   ./dfuGenPkg.sh -t altair -b
# to create bootloader dfu package for target altair
# or
#   ./dfuGenPkg.sh -t altair -f
# to create firmware (application) dfu package for target altair
###################################################################

# Get the scripts path: the path where this file is located.
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

usage() {
	echo "Options:"
	echo "   -f, --firmware                       add firmware   to dfu package"
	echo "   -b, --build                          add bootloader to dfu package"
	echo "   -s, --softdevice                     add softdevice to dfu package (not working yet)"
	echo "   -t target, --target target           specify config target"
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

SHORT=t:fbs
LONG=target:,firmware,bootloader,softdevice

PARSED=$(getopt --options $SHORT --longoptions $LONG --name "$0" -- "$@")
if [[ $? -ne 0 ]]; then
	exit $CS_ERR_GETOPT_PARSE
fi

eval set -- "$PARSED"


while true; do
	case "$1" in
		-b|--bootloader)
			add_bootloader=true
			shift 1
			;;
		-f|--firmware)
			add_firmware=true
			shift 1
			;;
		-s|--softdevice)
			add_softdevice=true
			shift 1
			;;
		-t|--target)
			target=$2
			shift 2
			;;
		--)
			shift
			break
			;;
		*)
			echo "Error in arguments."
			echo $2
			usage
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



args="--hw-version 52 --sd-req 0xB7"
pkg_name="_dfu.zip"

if [ $add_firmware ]; then
	if [ $add_softdevice ] || [ $add_bootloader ]; then
		cs_err "Can't add bootloader of softdevice in same package as firmware."
		exit 1
	fi
	args="$args --application $BLUENET_BIN_DIR/crownstone.hex --application-version 1"
	pkg_name="firmware${pkg_name}"
fi

if [ $add_softdevice ]; then
	args="$args --softdevice $BLUENET_BIN_DIR/softdevice_mainpart.hex"
	pkg_name="softdevice${pkg_name}"
fi

if [ $add_bootloader ]; then
	args="$args --bootloader $BLUENET_BIN_DIR/bootloader.hex --bootloader-version 1"
	pkg_name="bootloader${pkg_name}"
fi

cs_info "nrfutil pkg generate $args $BLUENET_BIN_DIR/${pkg_name}"
nrfutil pkg generate $args "$BLUENET_BIN_DIR/${pkg_name}"
