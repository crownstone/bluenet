#!/bin/bash

source _utils.sh

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_config.sh

SCRIPT_DIR=jlink
TEMP_DIR=$path/tmp
mkdir -p $TEMP_DIR

GDB_SCRIPT=gdbreset

GDB=${COMPILER_PATH}/bin/${COMPILER_TYPE}-gdb
#DEVICE=nrf51822

# Run JLink gdb server
$JLINK_GDB_SERVER -Device $DEVICE -If SWD -speed 400 &

# Stop running processes on exit of script
cleanup() {
	local pids=$(jobs -pr)
	[ -n "$pids" ] && kill $pids
}
trap "cleanup" INT QUIT TERM EXIT

log "$GDB -x ${path}/gdb/$GDB_SCRIPT"
$GDB -x $path/gdb/$GDB_SCRIPT

