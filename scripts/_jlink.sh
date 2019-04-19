#!/bin/bash

JLINK_SCRIPT=${1:? "$0 requires \"jlink script file\" as first argument"}
SERIAL_NUM=$2

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh
source $path/_config.sh

if [ -z $SERIAL_NUM ]; then
	cs_log "$JLINK -Device $DEVICE -speed 4000 -If SWD $JLINK_SCRIPT -ExitonError 1"
	$JLINK -Device $DEVICE -speed 4000 -If SWD $JLINK_SCRIPT -ExitonError 1
else
	cs_log "$JLINK -Device $DEVICE -speed 4000 -SelectEmuBySN $SERIAL_NUM -If SWD $JLINK_SCRIPT -ExitonError 1"
	$JLINK -Device $DEVICE -speed 4000 -SelectEmuBySN $SERIAL_NUM -If SWD $JLINK_SCRIPT -ExitonError 1
fi
