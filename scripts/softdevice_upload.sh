#!/bin/bash

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/config.sh

SCRIPT_DIR=$path/jlink
TEMP_DIR=$path/tmp
mkdir -p $TEMP_DIR

DEVICE=nrf51822

SOFTDEVICE_DIR=${1:? "$0 requires \"softdevice bin directory\" as first argument"}

SN=$2

if [ ! -e ${SOFTDEVICE_DIR} ]; then
	echo "Error: ${SOFTDEVICE_DIR} does not exist..."
	exit
fi

sed "s|@SOFTDEVICE_DIR@|$SOFTDEVICE_DIR|" $SCRIPT_DIR/softdevice.script > $TEMP_DIR/softdevice.script

if [ $SOFTDEVICE_NO_SEPARATE_UICR_SECTION == 1 ]; then
	echo "No UICR section, so remove line to flash it"
	sed -i '/uicr/d' $TEMP_DIR/softdevice.script
fi

if [ -z $2 ]; then
	$JLINK -Device $DEVICE -If SWD $TEMP_DIR/softdevice.script
else
	$JLINK -Device $DEVICE -SelectEmuBySN $2 -If SWD $TEMP_DIR/softdevice.script
fi
