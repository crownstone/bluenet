#!/bin/bash

# optional target, use crownstone as default
target=${1:-crownstone}

source _utils.sh

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# adjust targets and sets serial_num
# call it with source so that it get's the same arguments as the call to this script
# and so that the variables assigned in the script will be persistent afterwards
source ${path}/_check_targets.sh $target

# configure environment variables, load configuration files
source $path/_config.sh

SCRIPT_DIR=$path/jlink
TEMP_DIR=$path/tmp
mkdir -p $TEMP_DIR

#DEVICE=nrf51822

cp $SCRIPT_DIR/hardware_version.script $TEMP_DIR/hardware_version.script

if [ -z $serial_num ]; then
	log "$JLINK -Device $DEVICE -If SWD $TEMP_DIR/hardware_version.script -ExitonError 1"
	$JLINK -Device $DEVICE -speed 100 -If SWD $TEMP_DIR/hardware_version.script -ExitonError 1
else
	log "$JLINK -Device $DEVICE -SelectEmuBySN $serial_num -If SWD $TEMP_DIR/hardware_version.script -ExitonError 1"
	$JLINK -Device $DEVICE -speed 100 -SelectEmuBySN $serial_num -If SWD $TEMP_DIR/hardware_version.script -ExitonError 1
fi

checkError "Failed to get hardware version"

log "The result should be something like:"
log "    1000005C = 3C 00 FF FF"
log
log "This would mean HARDWARE_VERSION=0x003C in the config file"
log
log "This means version 003C in the document:"
log '    "Migrating to the latest nRF51822 chip version" "nWP-018" "White Paper v1.3"'

