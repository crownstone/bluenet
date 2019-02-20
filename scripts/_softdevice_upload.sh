#!/bin/bash

SOFTDEVICE_DIR=${1:? "$0 requires \"softdevice bin directory\" as first argument"}
SERIAL_NUM=$2

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh
source $path/_config.sh

SCRIPT_DIR=$path/jlink
TEMP_DIR=$path/tmp
mkdir -p $TEMP_DIR

if [ ! -e ${SOFTDEVICE_DIR} ]; then
	cs_err "Error: ${SOFTDEVICE_DIR} does not exist..."
	exit $CS_ERR_CONFIG
fi

sed "s|@SOFTDEVICE_DIR@|$SOFTDEVICE_DIR|" $SCRIPT_DIR/softdevice.script > $TEMP_DIR/softdevice.script

if [ $SOFTDEVICE_NO_SEPARATE_UICR_SECTION == 1 ]; then
	cs_log "No UICR section, so remove line to flash it"
	sed -i '/uicr/d' $TEMP_DIR/softdevice.script
fi

$path/jlink.sh $TEMP_DIR/softdevice.script $SERIAL_NUM
