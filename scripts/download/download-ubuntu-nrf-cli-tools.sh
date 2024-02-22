#!/bin/bash

usage="$0 <workspace path>"

WORKSPACE_DIR=${1:? "Usage: $usage"}

ARCH=$(dpkg --print-architecture)

NORDIC_DOWNLOAD_URL="https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/desktop-software/nrf-command-line-tools/sw/versions-10-x-x/10-24-0"

DEB_FILE="nrf-command-line-tools_10.24.0_${ARCH}.deb"
declare -A MD5
MD5=( ["armhf"]="8d52a5fb10e7ed7d93aefb2a143f9571" ["arm64"]="c218aad31c324aaea0ea535f0b844efd" ["amd64"]="426f5550963e57b0f897c3ae7f4136fb" )

DOWNLOAD_URL="$NORDIC_DOWNLOAD_URL/$DEB_FILE"

DOWNLOAD_DIR="$WORKSPACE_DIR/downloads"
mkdir -p "$DOWNLOAD_DIR"
cd "$DOWNLOAD_DIR"

SKIP_DOWNLOAD=0
if [ -e "$DEB_FILE" ]; then
	md5=($(md5sum "$DEB_FILE"))
	if [ "$md5" = "${MD5[$ARCH]}" ]; then
		SKIP_DOWNLOAD=1
	fi
fi

if [ $SKIP_DOWNLOAD -eq 1 ]; then
	echo "Skip download. Already file with proper checksum downloaded."
else
	wget --no-verbose --show-progress --progress=dot:giga --timestamping "$DOWNLOAD_URL"
	md5=($(md5sum "$DEB_FILE"))
	if [ "$md5" != "${MD5[$ARCH]}" ]; then
		echo "Checksum incorrect: $md5 != ${MD5[$ARCH]}"
		exit 1
	fi
fi

# Allow for prefix 'SUPERUSER_SWITCH= ' (empty) or 'SUPERUSER_SWITCH=su for systems without sudo
if [ -z ${SUPERUSER_SWITCH+set} ]; then
	SUPERUSER_SWITCH=sudo
fi

echo "Install $DEB_FILE file using apt"
$SUPERUSER_SWITCH apt -y install "./$DEB_FILE"
