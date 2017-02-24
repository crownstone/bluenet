#!/bin/bash

# Arguments to this script
option=${1:? "Usage: $0 (version|power|reboot)"}
	
# Optionally target as second argument to the script
target=${2:-default}

# Load scripts from the current directory. The path is defined in this way so that it can also be called from 
# another directory (and not only from this directory).
#   _utils.sh has color information
#   _config.sh has information about BLUENET variables
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh
source ${path}/_check_targets.sh $target
source $path/_config.sh
SCRIPT_DIR=jlink
TEMP_DIR=$path/tmp
mkdir -p $TEMP_DIR

# Run JLink gdb server
gdb_server() {
	info "Run JLink gdb server"
	GDB=${COMPILER_PATH}/bin/${COMPILER_TYPE}-gdb
	$JLINK_GDB_SERVER -Device $DEVICE -If SWD -speed 400 &
}

# Stop running processes on exit of script
cleanup() {
	local pids=$(jobs -pr)
	[ -n "$pids" ] && kill $pids
}
trap "cleanup" INT QUIT TERM EXIT

hardware_version() {
	cp $SCRIPT_DIR/hardware_version.script $TEMP_DIR/hardware_version.script

	info "Use jlink to run hardware_version.script (reads single memory location)"
	if [ -z $serial_num ]; then
		log "$JLINK -Device $DEVICE -If SWD $TEMP_DIR/hardware_version.script -ExitonError 1"
		$JLINK -Device $DEVICE -speed 100 -If SWD $TEMP_DIR/hardware_version.script -ExitonError 1
	else
		log "$JLINK -Device $DEVICE -SelectEmuBySN $serial_num -If SWD $TEMP_DIR/hardware_version.script -ExitonError 1"
		$JLINK -Device $DEVICE -speed 100 -SelectEmuBySN $serial_num -If SWD $TEMP_DIR/hardware_version.script -ExitonError 1
	fi

	checkError "Failed to get hardware version"

	log "The result should be something like:"
	log "    10000104 = 30 42 41 41"
	log
	log "This would mean the hardware version is AAB0 (read hex numbers from back to front)"
	log "See FICR in the product specification"
	log "B0 are the first 2 characters of the last 3 characters in the errata docs"
}

power() {
	info "Power up through JLink"
	SCRIPT=power.script
	$JLINK -Device $DEVICE -If SWD $SCRIPT_DIR/$SCRIPT
	checkError "Failed to power up"
}

reboot() {
	info "Reset through gdb"
	gdb_server
	GDB_SCRIPT=gdbreset
	log "$GDB -x ${path}/gdb/$GDB_SCRIPT"
	$GDB -x $path/gdb/$GDB_SCRIPT
	
	checkError "Failed to reboot"
}


case $option in
	power) 
		power
		;;
	reboot) 
		reboot
		;;
	version)
		hardware_version
		;;
	*)
		;;
esac

