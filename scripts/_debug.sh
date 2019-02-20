#!/bin/bash

TARGET=${1:? "$0 requires \"target\" as argument"}
SERIAL_NUM=$2
GDB_PORT=$3

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh
source $path/_config.sh

GDB_SCRIPT=$SCRIPT_TEMP_DIR/gdbinit
cp gdb/gdbinit $GDB_SCRIPT

GDB=${COMPILER_PATH}/bin/${COMPILER_TYPE}gdb

if [ -z $GDB_PORT ]; then
	GDB_PORT=2331
fi
SWO_PORT=$((${GDB_PORT} + 1))
TELNET_PORT=$((${SWO_PORT} + 1))

sed -i "s/@gdb_port@/${GDB_PORT}/" $GDB_SCRIPT

# Run JLink gdb server
if [ -z $SERIAL_NUM ]; then
	cs_log "$JLINK_GDB_SERVER -Device $DEVICE -If SWD -speed 4000 -port $GDB_PORT -swoport $SWO_PORT -telnetport $TELNET_PORT &"
	$JLINK_GDB_SERVER -Device $DEVICE -If SWD -speed 4000 -port $GDB_PORT -swoport $SWO_PORT -telnetport $TELNET_PORT &
else
	cs_log "$JLINK_GDB_SERVER -Device $DEVICE -select usb=$SERIAL_NUM -If SWD -speed 4000 -port $GDB_PORT -swoport $SWO_PORT -telnetport $TELNET_PORT &"
	$JLINK_GDB_SERVER -Device $DEVICE -select usb=$SERIAL_NUM -If SWD -speed 4000 -port $GDB_PORT -swoport $SWO_PORT -telnetport $TELNET_PORT &
fi

# Stop running processes on exit of script
cleanup() {
	local pids=$(jobs -pr)
	[ -n "$pids" ] && kill $pids
}
trap "cleanup" INT QUIT TERM EXIT

cs_log "$GDB -x $GDB_SCRIPT $TARGET"
$GDB -x $GDB_SCRIPT $TARGET
