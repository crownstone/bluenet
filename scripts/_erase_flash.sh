#!/bin/bash

SERIAL_NUM=$1

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh
source $path/_config.sh

SCRIPT_DIR=$path/jlink
TEMP_DIR=$path/tmp
mkdir -p $TEMP_DIR

DEVICE=nRF52832_xxAA

if [ -z $SERIAL_NUM ]; then
	cs_log "$JLINK -Device $DEVICE -If SWD $SCRIPT_DIR/erase.script -ExitonError 1"
	$JLINK -Device $DEVICE -speed 4000 -If SWD $SCRIPT_DIR/erase.script -ExitonError 1
else
	cs_log "$JLINK -Device $DEVICE -SelectEmuBySN $SERIAL_NUM -If SWD $SCRIPT_DIR/erase.script -ExitonError 1"
	$JLINK -Device $DEVICE -speed 4000 -SelectEmuBySN $SERIAL_NUM -If SWD $SCRIPT_DIR/erase.script -ExitonError 1
fi
