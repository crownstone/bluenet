#!/bin/bash

ADDRESS=${1:? "Requires address as argument"}
VALUE=$2
SERIAL_NUM=$3

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh
source $path/_config.sh

SCRIPT_DIR=$path/jlink
TEMP_DIR=$path/tmp
mkdir -p $TEMP_DIR

#DEVICE=nrf51822
DEVICE=nRF52832_xxAA

sed "s|@ADDRESS@|$ADDRESS|" $SCRIPT_DIR/writebyte.script > $TEMP_DIR/writebyte.script.1
sed "s|@VALUE@|$VALUE|" $TEMP_DIR/writebyte.script.1 > $TEMP_DIR/writebyte.script

if [ -z $SERIAL_NUM ]; then
	cs_log "$JLINK -Device $DEVICE -If SWD $TEMP_DIR/writebyte.script -ExitonError 1"
	$JLINK -Device $DEVICE -speed 4000 -If SWD $TEMP_DIR/writebyte.script -ExitonError 1
else
	cs_log "$JLINK -Device $DEVICE -SelectEmuBySN $SERIAL_NUM -If SWD $TEMP_DIR/writebyte.script -ExitonError 1"
	$JLINK -Device $DEVICE -speed 4000 -SelectEmuBySN $SERIAL_NUM -If SWD $TEMP_DIR/writebyte.script -ExitonError 1
fi
