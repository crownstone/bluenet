#!/bin/bash
#
# check if _targets.sh is present in the BLUENET_CONFIG_DIR. See _targets_template.sh for explanations

# Target if provided, otherwise default "crownstone"
target=${1:-crownstone}

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

if [[ -e "$BLUENET_CONFIG_DIR/_targets.sh" ]]; then
	# Adjusts target and sets serial_num and gdb_port
	source $BLUENET_CONFIG_DIR/_targets.sh $target
	target_set=true
else
	if [[ $target != "crownstone" ]]; then
		cs_err "To use different targets, copy _targets_template.sh to $BLUENET_CONFIG_DIR and rename to _targets.sh"
		exit 1
	else
		cs_log "Not found: $BLUENET_CONFIG_DIR/_targets.sh"
		cs_log "You can ignore this when config dir was already changed to correct target."
		cs_warn "NOTE: this is the old way of calling the scripts, you might want to check _targets_template.sh or the README"
	fi
fi
