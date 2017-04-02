#!/bin/bash

#####################################################################
# build a specific release in the $BLUENET_DIR/release directory
# with ./build_release.sh target, e.g.
#   ./build_release.sh crownstone_1.2.0
# the binaries for firmware and bootloader, as well as dfu packages
# will be copied to $BLUENET_BIN_DIR/target
#
# or use ./build_release.sh all to build all releases in the
# $BLUENET_DIR/release folder
#####################################################################

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $BLUENET_DIR/scripts/_utils.sh

OLD_BLUENET_BUILD_DIR=$BLUENET_BUILD_DIR
OLD_BLUENET_BIN_DIR=$BLUENET_BIN_DIR

buildOne() {
	target=$1

	# modify paths
	export BLUENET_CONFIG_DIR=$BLUENET_DIR/release/$target
	export BLUENET_BUILD_DIR=$OLD_BLUENET_BUILD_DIR/$target
	export BLUENET_BIN_DIR=$OLD_BLUENET_BIN_DIR/$target

	pushd $BLUENET_DIR/scripts

	echo `pwd`

	cs_info "Build firmware for target $target ..."
	./firmware.sh release

	checkError
	cs_succ "Build DONE"

	cs_info "Build bootloader ..."
	${BLUENET_BOOTLOADER_DIR}/scripts/all.sh

	checkError
	cs_succ "Build DONE"

	cs_info "Create DFU packets"

	./dfuGenPkg.py -a "$BLUENET_BIN_DIR/crownstone.hex" -b "$BLUENET_BIN_DIR/bootloader_dfu.hex"

	popd
}

buildAll() {

	pushd $BLUENET_DIR/release
	for i in `ls -d */`; do
		buildOne ${i%%/}
	done

}

target=${1:? "Usage: $0 \"target|all\""}

if [[ $target == "all" ]]; then
	buildAll
else
	buildOne $target
fi
