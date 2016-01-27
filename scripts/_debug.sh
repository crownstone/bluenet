#!/bin/bash

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_config.sh

SCRIPT_DIR=$path/jlink
TEMP_DIR=$path/tmp
mkdir -p $TEMP_DIR

GDB_SCRIPT=$TEMP_DIR/gdbinit
cp gdb/gdbinit $GDB_SCRIPT


GDB=${COMPILER_PATH}/bin/${COMPILER_TYPE}-gdb
DEVICE=nrf51822

TARGET=${1:? "$0 requires \"target\" as first argument"}

SN=$2
GDB_PORT=$3
if [ -z $GDB_PORT ]; then
	GDB_PORT=2331
fi
SWO_PORT=$((${GDB_PORT} + 1))
TELNET_PORT=$((${SWO_PORT} + 1))

sed -i "s/@gdb_port@/${GDB_PORT}/" $GDB_SCRIPT

# Run JLink gdb server
if [ -z $SN ]; then
	echo "1: $JLINK_GDB_SERVER -Device $DEVICE -If SWD -speed 400 -port $GDB_PORT -swoport $SWO_PORT -telnetport $TELNET_PORT &"
	$JLINK_GDB_SERVER -Device $DEVICE -If SWD -speed 400 -port $GDB_PORT -swoport $SWO_PORT -telnetport $TELNET_PORT &
else
	echo "2: $JLINK_GDB_SERVER -Device $DEVICE -select usb=$SN -If SWD -speed 400 -port $GDB_PORT -swoport $SWO_PORT -telnetport $TELNET_PORT &"
	$JLINK_GDB_SERVER -Device $DEVICE -select usb=$SN -If SWD -speed 400 -port $GDB_PORT -swoport $SWO_PORT -telnetport $TELNET_PORT &
fi

# Stop running processes on exit of script
cleanup() {
	local pids=$(jobs -pr)
	[ -n "$pids" ] && kill $pids
}
trap "cleanup" INT QUIT TERM EXIT

echo "$GDB -x $GDB_SCRIPT $TARGET"
$GDB -x $GDB_SCRIPT $TARGET

