#!/bin/bash

SCRIPT_DIR=jlink
mkdir -p tmp
TEMP_DIR=tmp

source config.sh
DEVICE=nrf51822

SOFTDEVICE_DIR=${1:? "$0 requires \"softdevice bin directory\" as first argument"}

if [ ! -e ${SOFTDEVICE_DIR} ]; then
	echo "Error: ${SOFTDEVICE_DIR} does not exist..."
	exit
fi

sed "s|@SOFTDEVICE_DIR@|$SOFTDEVICE_DIR|" $SCRIPT_DIR/softdevice.script > $TEMP_DIR/softdevice.script

if [ $SOFTDEVICE_NO_SEPARATE_UICR_SECTION == 1 ]; then
	echo "No UICR section, so remove line to flash it"
	sed -i '/uicr/d' $TEMP_DIR/softdevice.script
fi 

$JLINK -Device $DEVICE -If SWD $TEMP_DIR/softdevice.script 
