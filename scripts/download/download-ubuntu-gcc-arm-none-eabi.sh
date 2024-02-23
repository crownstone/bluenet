#!/bin/bash

usage="$0 <workspace path>"

WORKSPACE_DIR=${1:? "Usage: $usage"}

ARCH=$(dpkg --print-architecture)
if [ "$ARCH" == "arm64" ]; then
	ARCH="aarch64"
fi

if [[ "$ARCH" != "aarch64" && "$ARCH" != "x86_64" ]]; then
	echo "Architecture not supported by download script. Maybe you'll find a fit at https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases/tag/v10.3.1-2.2"
	exit 1
fi

# After extraction
DOWNLOAD_DIR="gcc-arm-none-eabi-10.3-2021.10"

DOWNLOAD_FILE="gcc-arm-none-eabi-10.3-2021.10-${ARCH}-linux.tar.bz2"
DOWNLOAD_URL="https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/$DOWNLOAD_FILE"
declare -A MD5
MD5=( ["aarch64"]="3fe3d8bb693bd0a6e4615b6569443d0d" ["x86_64"]="2383e4eb4ea23f248d33adc70dc3227e" )

TOOLS_DIR="$WORKSPACE_DIR/tools"
mkdir -p "$TOOLS_DIR"
cd "$TOOLS_DIR"

SKIP_DOWNLOAD=0
if [ -e "$DOWNLOAD_FILE" ]; then
	md5=($(md5sum "$DOWNLOAD_FILE"))
	if [ "$md5" = "${MD5[$ARCH]}" ]; then
		SKIP_DOWNLOAD=1
	fi
fi

if [ $SKIP_DOWNLOAD -eq 1 ]; then
	echo "Skip download. Already file with proper checksum downloaded."
else
	wget --no-verbose --show-progress --progress=dot:giga --timestamping "$DOWNLOAD_URL"
	md5=($(md5sum "$DOWNLOAD_FILE"))
	if [ "$md5" != "${MD5[$ARCH]}" ]; then
		echo "Checksum incorrect: $md5 != ${MD5[$ARCH]}"
		exit 1
	fi
	echo "Extract file"
	tar -xf $DOWNLOAD_FILE --checkpoint=.100
	echo "Create symlink"
	rm -f gcc_arm_none_eabi
	ln -s "$DOWNLOAD_DIR" gcc_arm_none_eabi
fi

