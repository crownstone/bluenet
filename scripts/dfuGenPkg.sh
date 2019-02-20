#!/bin/bash

###################################################################
# uses targets , e.g.
#   ./dfuGenPkg.sh -t altair -b
# to create bootloader dfu package for target altair
# or
#   ./dfuGenPkg.sh -t altair -f
# to create firmware (application) dfu package for target altair
#
# use dfuGenPkg.py if you want to specify the hex files yourself
###################################################################

usage() {
	echo "Options:"
	echo "   -f                                   make a firmware dfu package"
	echo "   -b                                   make a bootloader dfu package"
#	echo "   -s                                   make a softdevice dfu package"
	echo "   -t target                            specify config target"
#	echo "   -S                                   softdevice required"
}

dfu_target="none"

args=""

while getopts "afbst:S" optname; do
	case "$optname" in
		"b")
			dfu_target="bootloader_dfu"
			args="$args -b"
			# shift $((OPTIND-1))
			;;
		"a"|"f")
			dfu_target="firmware"
			args="$args -a"
			;;
		"t")
			target=$2
			#. $BLUENET_CONFIG_DIR/_targets.sh $OPTARG
			# shift $((OPTIND-1))
			;;
		*)
			usage
			exit 1
			;;
	esac
done

# Default target
target=${target:-crownstone}

# Get the scripts path: the path where this file is located.
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

# Adjust config, build, bin, and release dirs. Assign serial_num and gdb_port from target.
cs_info "Load configuration from: ${path}/_check_targets.sh $target"
source ${path}/_check_targets.sh $target

# Check environment variables, load configuration files.
cs_info "Load configuration from: ${path}/_config.sh"
source $path/_config.sh

if [ $dfu_target == "none" ]; then
	cs_err "Specify what dfu package to generate"
	exit $CS_ERR_ARGUMENTS
fi

if [ $dfu_target == "firmware" ]; then
	cs_info "Set target to $target"
	dfu_target=$target
fi

cs_log "./dfuGenPkg.py $args $BLUENET_BIN_DIR/$dfu_target.hex"
$path/dfuGenPkg.py $args $BLUENET_BIN_DIR/$dfu_target.hex
