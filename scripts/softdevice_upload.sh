#!/bin/sh

SCRIPT_DIR=jlink
mkdir -p tmp
TEMP_DIR=tmp

JLINK=/usr/bin/JLinkExe
DEVICE=nrf51822

SOFTDEVICE_DIR=${1:? "$0 requires \"softdevice directory\" as first argument"}

if [ ! -e ${SOFTDEVICE_DIR} ]; then
	echo "Error: ${SOFTDEVICE_DIR} does not exist..."
	exit
fi

sed "s|@SOFTDEVICE_DIR@|$SOFTDEVICE_DIR|" $SCRIPT_DIR/softdevice.script > $TEMP_DIR/softdevice.script
$JLINK -Device $DEVICE -If SWD $TEMP_DIR/softdevice.script 
