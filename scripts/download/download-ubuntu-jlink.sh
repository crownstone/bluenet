#!/bin/bash

usage="$0 <workspace path>"

WORKSPACE_DIR=${1:? "Usage: $usage"}

ARCH=$(dpkg --print-architecture)
if [ "$ARCH" == "armhf" ]; then
	ARCH="arm"
fi

SEGGER_DOWNLOAD_URL=https://www.segger.com/downloads/jlink

DEB_FILE="JLink_Linux_V794k_${ARCH}.deb"
declare -A MD5
MD5=( ["arm"]="4c2ccd326cae510b0798c525e705577b" ["arm64"]="1f895a0ea4aeba658d1247ce335b4d9e" ["i386"]="c39f3e1f581da2e712709163d5fa6b76" ["x86_64"]="e001bf6e53f1bab1c44438dc5fefb38a" )

DOWNLOAD_URL="$SEGGER_DOWNLOAD_URL/$DEB_FILE"

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
	wget --no-verbose --show-progress --progress=dot:giga --timestamping --post-data "accept_license_agreement=accepted&non_emb_ctr=confirmed" "$DOWNLOAD_URL"
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
