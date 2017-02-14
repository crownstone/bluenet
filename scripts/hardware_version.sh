#!/bin/bash

# optional target, use crownstone as default
target=${1:-crownstone}

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

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
log "    10000104 = 30 42 41 41"
log
log "This would mean the hardware version is AAB0 (read hex numbers from back to front)"
log "See FICR in the product specification"
log "B0 are the first 2 characters of the last 3 characters in the errata docs"
