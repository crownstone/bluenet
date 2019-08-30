#!/bin/bash

# Usage: _debug.sh <file.elf> [target [extra_elf_file extra_elf_file_address]]

ELF_FILE=${1:? "$0 requires elf file as argument"}

# optional target, use crownstone as default
target=${2:-crownstone}

# Get the scripts path: the path where this file is located.
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

cs_info "Load configuration from: ${path}/_check_targets.sh $target"
source ${path}/_check_targets.sh $target

cs_info "Load configuration from: ${path}/_config.sh"
source $path/_config.sh


SERIAL_NUM=$serial_num
GDB_PORT=$gdb_port

EXTRA_ELF_FILE=$3
EXTRA_ELF_FILE_ADDR=$4

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

if [ $EXTRA_ELF_FILE ] && [ $EXTRA_ELF_FILE_ADDR ]; then
	EXTRA_ELF_FILE_ESCAPED=$(echo $EXTRA_ELF_FILE | sed -e 's/[\/&]/\\&/g')
	sed -i "s/@add-symbol-file@/add-symbol-file ${EXTRA_ELF_FILE_ESCAPED} ${EXTRA_ELF_FILE_ADDR}/" $GDB_SCRIPT
else
	sed -i "s/@add-symbol-file@//" $GDB_SCRIPT
fi

# Run JLink gdb server
if [ -z $SERIAL_NUM ]; then
	cs_log "$JLINK_GDB_SERVER -Device $DEVICE -If SWD -speed 4000 -port $GDB_PORT -swoport $SWO_PORT -telnetport $TELNET_PORT &"
	$JLINK_GDB_SERVER -Device $DEVICE -If SWD -speed 4000 -port $GDB_PORT -swoport $SWO_PORT -telnetport $TELNET_PORT > /dev/null 2>&1 &
else
	cs_log "$JLINK_GDB_SERVER -Device $DEVICE -select usb=$SERIAL_NUM -If SWD -speed 4000 -port $GDB_PORT -swoport $SWO_PORT -telnetport $TELNET_PORT &"
	$JLINK_GDB_SERVER -Device $DEVICE -select usb=$SERIAL_NUM -If SWD -speed 4000 -port $GDB_PORT -swoport $SWO_PORT -telnetport $TELNET_PORT > /dev/null 2>&1 &
fi

# Stop running processes on exit of script
cleanup() {
	local pids=$(jobs -pr)
	cs_info "Clean up all jobs ($pids)"
	[ -n "$pids" ] && kill $pids
}
trap "cleanup" INT QUIT TERM EXIT

cs_log "$GDB -x $GDB_SCRIPT $ELF_FILE"
$GDB -x $GDB_SCRIPT $ELF_FILE
