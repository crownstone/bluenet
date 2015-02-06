#!/bin/bash

cmd=${1:? "$0 requires \"cmd\" as first argument"}

# get working path in absolute sense
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
working_path=$path

SD_BINDIR=${path}/../build/softdevice

config_file=CMakeBuild.config
if [ $cmd != "switch" ]; then
	echo "Load SoftDevice directory from configuration file"
	source ../$config_file
fi

build() {
	echo "There is no real building step. Nordic provides a binary blob as SoftDevice"
	echo "However, we still need to extract the binary and the config blob"
	echo "  from $SOFTDEVICE_DIR/$SOFTDEVICE"
	./softdevice_objcopy.sh $SOFTDEVICE_DIR $SD_BINDIR $SOFTDEVICE $COMPILER_PATH $COMPILER_TYPE $SOFTDEVICE_NO_SEPARATE_UICR_SECTION
}

upload() {
	./softdevice_upload.sh $SD_BINDIR
}

switch() {
	cd ..
	if [ -z "$1" ]; then
		echo "Parameter required: s110 or s130"
		return
	fi
	echo "Switch configuration file to: $1"
	rm -f $config_file
	ln -s $config_file.$1 $config_file
	ls -latr | grep $config_file | tail -n1
	#source $config_file
	#echo "Set current softdevice to: $SOFTDEVICE"
}

all() {
	build
	sleep 1
	upload
}

case "$cmd" in 
	build)
		build
		;;
	upload)
		upload
		;;
	switch)
		shift
		switch $@
		;;
	all)
		all
		;;
	*)
		echo $"Usage: $0 {build|upload|switch|all}"
		exit 1
esac

