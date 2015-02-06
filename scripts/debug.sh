#!/bin/bash

SCRIPT_DIR=jlink
mkdir -p tmp
TEMP_DIR=tmp

GDB_SCRIPT=gdbinit

source config.sh

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

echo "$GDB -x gdb/$GDB_SCRIPT $TARGET"
$GDB -x gdb/$GDB_SCRIPT $TARGET

