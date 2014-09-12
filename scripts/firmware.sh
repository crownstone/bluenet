#!/bin/sh

cmd=${1:? "$0 requires \"cmd\" as first argument"}
target=${2:? "$0 requires \"target\" as second argument"}

# get working path in absolute sense
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
working_path=$path

# todo: add more code to check if target exists

build() {
	cd $working_path/..	
	make all
	cd $working_path
}

upload() {
	./upload.sh ../build/$target.bin
}

debug() {
	./debug.sh ../build/$target.elf
}

all() {
	build
	sleep 1
	upload
	sleep 1
	debug
}

case "$cmd" in 
	build)
		build
		;;
	upload)
		upload
		;;
	debug)
		debug
		;;
	all)
		all
		;;
	*)
		echo $"Usage: $0 {build|upload|debug|all}"
		exit 1
esac

