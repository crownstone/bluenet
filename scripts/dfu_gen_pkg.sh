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
	echo "   -b, --bootloader                     add bootloader to dfu package"
	echo "   -s, --softdevice                     add softdevice to dfu package (not working yet)"
	echo "   -t target, --target target           specify config target"
	echo "   -F hexfile, --fwhex hexfile          add specified file as firmware   to dfu package"
	echo "   -B hexfile, --blhex hexfile          add specified file as bootloader to dfu package"
	echo "   -S hexfile, --sdhex hexfile          add specified file as softdevice to dfu package"
	echo "   -k keyfile, --key keyfile            add a keyfile to sign the dfu package"
	echo "   -K, --key-from-pass                  add key from pass"
	echo "   -v version, --version version        add a version int to the dfu package"
	echo "   -o filename, --output filename       use given filename as output file"
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

SHORT=t:k:v:F:B:S:o:fbsK
LONG=target:key:version:fwhex:blhex:sdhex:output:,firmware,bootloader,softdevice,key-from-pass

PARSED=$(getopt --options $SHORT --longoptions $LONG --name "$0" -- "$@")
if [[ $? -ne 0 ]]; then
	exit $CS_ERR_GETOPT_PARSE
fi

eval set -- "$PARSED"


while true; do
	case "$1" in
		-f|--firmware)
			add_firmware=true
			shift 1
			;;
		-b|--bootloader)
			add_bootloader=true
			shift 1
			;;
		-s|--softdevice)
			add_softdevice=true
			shift 1
			;;
		-F|--fwhex)
			add_firmware=true
			firmware_hex="$2"
			shift 2
			;;
		-B|--blhex)
			add_bootloader=true
			bootloader_hex="$2"
			shift 2
			;;
		-S|--sdhex)
			add_softdevice=true
			softdevice_hex="$2"
			shift 2
			;;
		-t|--target)
			target="$2"
			shift 2
			;;
		-k|--key)
			key_file="$2"
			shift 2
			;;
		-K|--key-from-pass)
			key_from_pass=true
			shift 1
			;;
		-v|--version)
			version_int=$2
			shift 2
			;;
		-o|--output)
			output_file="$2"
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

if [ $key_from_pass ]; then
	key_file=$(mktemp)
	trap "rm -f $key_file" EXIT
	pass dfu-pkg-sign-key > $key_file
fi

if [ $add_firmware ]; then
	if [ $add_softdevice ] || [ $add_bootloader ]; then
		cs_err "Can't add bootloader of softdevice in same package as firmware."
		exit 1
	fi
	firmware_hex=${firmware_hex:-$BLUENET_BIN_DIR/crownstone.hex}
	args="$args --application $firmware_hex"
	pkg_name="firmware${pkg_name}"
	
	if [ $version_int ]; then
		args="$args --application-version $version_int"
	else
		args="$args --application-version 1"
	fi
fi

if [ $add_softdevice ]; then
	if [ ! $key_file ]; then
		cs_err "You need to sign a package with softdevice."
		exit 1
	fi
	softdevice_hex=${softdevice_hex:-$BLUENET_BIN_DIR/softdevice_mainpart.hex}
	args="$args --softdevice $softdevice_hex"
	pkg_name="softdevice${pkg_name}"
fi

if [ $add_bootloader ]; then
	if [ ! $key_file ]; then
		cs_err "You need to sign a package with bootloader."
		exit 1
	fi
	bootloader_hex=${bootloader_hex:-$BLUENET_BIN_DIR/bootloader.hex}
	args="$args --bootloader $bootloader_hex"
	pkg_name="bootloader${pkg_name}"
	if [ $version_int ]; then
		args="$args --bootloader-version $version_int"
	else
		cs_err "Bootloader needs a version int"
		exit 1
	fi
fi

if [ $key_file ]; then
	args="$args --key-file $key_file"
fi

output_file=${output_file:-$BLUENET_BIN_DIR/${pkg_name}}
cs_info "nrfutil pkg generate $args $output_file"
nrfutil pkg generate $args "$output_file"
