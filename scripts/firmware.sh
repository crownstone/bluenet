#!/bin/bash

cmd=${1:? "Usage: $0 \"cmd\", \"target\""}
target=${2:? "Usage: $0 \"cmd\", \"target\""}

# optional address
address=$3

# get working path in absolute sense
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
working_path=$path

# todo: add more code to check if target exists

build() {
	cd $working_path/..	
	make all
	result=$?
	cd $working_path
	return $result
}

upload() {
	./upload.sh ../build/$target.bin $address
}

debug() {
	./debug.sh ../build/$target.elf
}

all() {
	build
	if [ $? -eq 0 ]; then 
		sleep 1
		upload
		sleep 1
		debug
	fi
}

run() {
	build
	if [ $? -eq 0 ]; then 
		sleep 1
		upload
	fi
}

clean() {
	cd ..
	make clean
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
	run)
		run
		;;
	clean)
		clean
		;;
	*)
		echo $"Usage: $0 {build|upload|debug|clean|run|all}"
		exit 1
esac

