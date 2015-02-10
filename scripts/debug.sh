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

# Run JLink gdb server
$JLINK_GDB_SERVER -Device $DEVICE -If SWD -speed 400 &

# Stop running processes on exit of script
cleanup() {
	local pids=$(jobs -pr)
	[ -n "$pids" ] && kill $pids
}
trap "cleanup" INT QUIT TERM EXIT

echo "$GDB -x ${path}/gdb/$GDB_SCRIPT $TARGET"
$GDB -x ${path}/gdb/$GDB_SCRIPT $TARGET

