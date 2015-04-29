#!/bin/bash

cmd=${1:? "Usage: $0 \"cmd\", \"target\""}

if [[ $cmd != "help" && $cmd != "init" && $cmd != "combined" && $cmd != "connect" ]]; then
	target=${2:? "Usage: $0 \"cmd\", \"target\""}
fi

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/config.sh

debug_target=$target.elf
binary_target=$target
combined_target=combined.hex

init() {
	openocd -d3 -f ${path}/openocd/openocd.cfg
}

connect() {
	telnet 127.0.0.1 4444
}

upload() {
	cd $path/telnet
	./flash.expect app $binary_target $APPLICATION_START_ADDRESS $BLUENET_CONFIG_DIR/build
}

# Flash application and firmware as combined binary
combined() {
	cd $path/telnet
	./flash.expect all $combined_target 0 $BLUENET_CONFIG_DIR/build
}

debug() {
	${COMPILER_PATH}/bin/${COMPILER_TYPE}-gdb -tui ${BLUENET_CONFIG_DIR}/build/$debug_target
}

help() {
	echo $"Usage: $0 {init|connect|upload|combined|debug}"
}

case "$cmd" in
	init)
		init
		;;
	connect)
		connect
		;;
	upload)
		upload
		;;
	combined)
		combined
		;;
	debug)
		debug
		;;
	help)
		help
		;;
	*)
		echo $"Usage: $0 {init|connect|upload|combined|debug}"
		exit 1
esac


