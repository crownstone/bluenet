#!/bin/bash

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/config.sh

SCRIPT_DIR=$path/jlink
TEMP_DIR=$path/tmp
mkdir -p $TEMP_DIR

GDB_SCRIPT=gdbinit

GDB=${COMPILER_PATH}/bin/${COMPILER_TYPE}-gdb
DEVICE=nrf51822

TARGET=${1:? "$0 requires \"target\" as first argument"}

SN=$2

# Run JLink gdb server
if [ -z $SN ]; then
	echo 1: $JLINK_GDB_SERVER -Device $DEVICE -If SWD -speed 400 &
	$JLINK_GDB_SERVER -Device $DEVICE -If SWD -speed 400 &
else
	echo 2: $JLINK_GDB_SERVER -Device $DEVICE -select usb=$SN -If SWD -speed 400 &
	$JLINK_GDB_SERVER -Device $DEVICE -select usb=$SN -If SWD -speed 400 &
fi

# Stop running processes on exit of script
cleanup() {
	local pids=$(jobs -pr)
	[ -n "$pids" ] && kill $pids
}
trap "cleanup" INT QUIT TERM EXIT

echo "$GDB -x ${path}/gdb/$GDB_SCRIPT $TARGET"
$GDB -x ${path}/gdb/$GDB_SCRIPT $TARGET

