#!/bin/bash

usage() {
	echo "Usage: $0 {build|unit-test-host|unit-test-nrf5|release|upload|debug|clean} [target [address|gdb_port]]"
}

cmd=${1:-help}
if [[ $cmd == "help" ]]; then
	usage
	exit 0
fi

# optional target, use crownstone as default
target=${2:-crownstone}

# Get the scripts path: the path where this file is located.
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

cs_info "Load configuration from: ${path}/_check_targets.sh $target"
source ${path}/_check_targets.sh $target

cs_info "Load configuration from: ${path}/_config.sh"
source $path/_config.sh

# optional address, use APPLICATION_START_ADDRESS as default
address=${3:-$APPLICATION_START_ADDRESS}

gdb_port=${3:-$gdb_port}

if [ "$VERBOSE" == "1" ]; then
	cs_info "Verbose mode"
	make_flag=""
else
	cs_info "Silent mode"
	make_flag="-s"
fi

# use $APPLICATION_START_ADDRESS as default if no address defined
address=${address:-$APPLICATION_START_ADDRESS}



build() {
	cd ${path}/..
	cs_info "Execute make cross-compile-target"
	make ${make_flag} cross-compile-target
	checkError "Build failed"
	cd $path
}

unit-test-host() {
	cd ${path}/..
	cs_info "Execute make host-compile-target"
	make ${make_flag} host-compile-target
	checkError "Failed to build unit test host"
	cd $path
}

release() {
	cd ${path}/..
	cs_info "Execute make release"
	make ${make_flag} release
	checkError "Failed to build release"
	cd $path
}

upload() {
	${path}/_upload.sh $BLUENET_BIN_DIR/crownstone.hex $address $serial_num
	checkError "Uploading failed"
}

debug() {
	${path}/_debug.sh $BLUENET_BIN_DIR/crownstone.elf $serial_num $gdb_port
	checkError "Debugging failed"
}

clean() {
	cd ${path}/..
	make ${make_flag} clean
	checkError "Cleanup failed"
}

case "$cmd" in
	build|unit-test-nrf5)
		build
		;;
	unit-test-host)
		unit-test-host
		;;
	release)
		release
		;;
	upload)
		upload
		;;
	debug)
		debug
		;;
	clean)
		clean
		;;
	*)
		usage
		exit $CS_ERR_ARGUMENTS
esac

