#!/bin/bash
#
# check if _targets.sh is present in the BLUENET_CONFIG_DIR. See _targets_template.sh for explanations

if [[ -z $2 ]]; then
	# if no target provided, set default target to crownstone
	target="crownstone"
else
	target=$2
fi

if [[ -e "$BLUENET_CONFIG_DIR/_targets.sh" ]]; then
	#adjusts target and sets serial_num
	. $BLUENET_CONFIG_DIR/_targets.sh $target
else
	if [[ $target != "crownstone" ]]; then
		echo "To use different targets, copy _targets_template.sh to $BLUENET_CONFIG_DIR and rename to _targets.sh"
		exit 1
	fi
fi
