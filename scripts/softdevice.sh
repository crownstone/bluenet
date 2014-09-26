#!/bin/sh

cmd=${1:? "$0 requires \"cmd\" as first argument"}

# get working path in absolute sense
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
working_path=$path

config_file=CMakeBuild.config

echo "Load SoftDevice directory from configuration file"
source ../$config_file

build() {
	echo "There is no real building step. Nordic provides a binary blob as SoftDevice"
	echo "However, we still need to extract the binary and the config blob"
	echo "  from $SOFTDEVICE_DIR/$SOFTDEVICE"
	./softdevice_objcopy.sh $SOFTDEVICE_DIR $SOFTDEVICE $COMPILER_PATH $COMPILER_TYPE $SOFTDEVICE_NO_SEPARATE_UICR_SECTION
}

upload() {
	./softdevice_upload.sh $SOFTDEVICE_DIR
}

switch() {
	cd ..
	echo "Switch configuration file to: $1"
	rm $config_file
	ln -s $config_file.$1 $config_file
	ls -latr | grep $config_file
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

