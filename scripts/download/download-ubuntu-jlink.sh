#!/bin/bash

usage="$0 <workspace path>"

WORKSPACE_DIR=${1:? "Usage: $usage"}

SEGGER_DOWNLOAD_URL=https://www.segger.com/downloads/jlink

DEB_FILE=JLink_Linux_V760b_x86_64.deb
MD5=4b0e24a040a74117a041807536372846

DOWNLOAD_URL="$SEGGER_DOWNLOAD_URL/$DEB_FILE"

DOWNLOAD_DIR="$WORKSPACE_DIR/downloads"
mkdir -p "$DOWNLOAD_DIR"
cd "$DOWNLOAD_DIR"

SKIP_DOWNLOAD=0
if [ -e "$DEB_FILE" ]; then
	md5=($(md5sum "$DEB_FILE"))
	if [ "$md5" = "$MD5" ]; then
		SKIP_DOWNLOAD=1
	fi
fi

if [ $SKIP_DOWNLOAD -eq 1 ]; then
	echo "Skip download. Already file with proper checksum downloaded."
else
	wget --timestamping --post-data "accept_license_agreement=accepted&non_emb_ctr=confirmed" "$DOWNLOAD_URL"
	md5=($(md5sum "$DEB_FILE"))
	if [ "$md5" != "$MD5" ]; then
		echo "Checksum incorrect: $md5 != $MD5"
		exit 1
	fi
fi

echo "Install $DEB_FILE file using apt (sudo rights required)"
sudo apt -y install "./$DEB_FILE"
