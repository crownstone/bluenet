#!/bin/bash

dfu_target="crownstone"

args=""

while getopts "bt:ai:vd" optname; do
	case "$optname" in
		"b")
			dfu_target="bootloader_dfu"
			args="$args -b"
			# shift $((OPTIND-1))
			;;
		"a")
			dfu_target="crownstone"
			args="$args -a"
			;;
		"t")
			. $BLUENET_CONFIG_DIR/_targets.sh $OPTARG
			# shift $((OPTIND-1))
			;;
		*)
			;;
	esac
done

echo "./dfuGenPkg.py -f $BLUENET_BIN_DIR/$dfu_target.hex $args"
./dfuGenPkg.py $args $BLUENET_BIN_DIR/$dfu_target.hex
