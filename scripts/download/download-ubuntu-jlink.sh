#!/bin/bash

usage="$0 <workspace path>"

WORKSPACE_DIR=${1:? "Usage: $usage"}

JLINK_DOWNLOAD_URL=https://www.segger.com/downloads/jlink

JLINK_DEB_FILE=JLink_Linux_V760b_x86_64.deb
JLINK_MD5=4b0e24a040a74117a041807536372846

JLINK_LINUX_DOWNLOAD_URL="$JLINK_DOWNLOAD_URL/$JLINK_DEB_FILE"

DOWNLOAD_DIR="$WORKSPACE_DIR/download"
mkdir -p "$DOWNLOAD_DIR"
cd "$DOWNLOAD_DIR"

SKIP_DOWNLOAD=0
if [ -e "$JLINK_DEB_FILE" ]; then
	md5=($(md5sum "$JLINK_DEB_FILE"))
	if [ "$md5" = "$JLINK_MD5" ]; then
		SKIP_DOWNLOAD=1
	fi
fi

if [ $SKIP_DOWNLOAD -eq 1 ]; then
	echo "Skip download. Already file with proper checksum downloaded."
else
	wget --timestamping --post-data "accept_license_agreement=accepted&non_emb_ctr=confirmed" "$JLINK_LINUX_DOWNLOAD_URL"
	md5=($(md5sum "$JLINK_DEB_FILE"))
	if [ "$md5" = "$JLINK_MD5" ]; then
		echo "Checksum incorrect"
		exit 1
	fi
fi

echo "Install $JLINK_DEB_FILE file using apt (sudo rights required)"
sudo apt install "./$JLINK_DEB_FILE"
