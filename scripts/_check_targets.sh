#!/bin/bash
#
# check if _targets.sh is present in the BLUENET_CONFIG_DIR. See _targets_template.sh for explanations

# if [[ -z $2 ]]; then
# 	# if no target provided, set default target to crownstone
# 	target="crownstone"
# else
# 	target=$2
# fi

#target if provided, otherwise default "crownstone"
target=${1:-crownstone}

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

if [[ -e "$BLUENET_CONFIG_DIR/_targets.sh" ]]; then
	#adjusts target and sets serial_num
	source $BLUENET_CONFIG_DIR/_targets.sh $target
else
	if [[ $target != "crownstone" ]]; then
		err "To use different targets, copy _targets_template.sh to $BLUENET_CONFIG_DIR and rename to _targets.sh"
		exit 1
	else
		warn "NOTE: this is the old way of calling the scripts, you might want to check _targets_template.sh or the README"
	fi
fi
