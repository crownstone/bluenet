#!/bin/bash

usage="$0 <workspace path>"

WORKSPACE_DIR=${1:? "Usage: $usage"}

NORDIC_DOWNLOAD_URL="https://www.nordicsemi.com/-/media/Software-and-other-downloads"

DOWNLOAD_FILE="nRF-Command-Line-Tools_10_14_0_Linux64.zip"
DOWNLOAD_URL="${NORDIC_DOWNLOAD_URL}/Desktop-software/nRF-command-line-tools/sw/Versions-10-x-x/10-14-0/$DOWNLOAD_FILE"
DEB_FILE="nrf-command-line-tools_10.14.0_amd64.deb"
MD5="8a049bacc67519561b77e014b652d5df"

ZIP_FOLDER=./nRF-Command-Line-Tools_10_14_0_Linux64

DOWNLOAD_DIR="$WORKSPACE_DIR/downloads"
mkdir -p "$DOWNLOAD_DIR"
cd "$DOWNLOAD_DIR"

SKIP_DOWNLOAD=0
if [ -e "$DOWNLOAD_FILE" ]; then
	md5=($(md5sum "$DOWNLOAD_FILE"))
	if [ "$md5" = "$MD5" ]; then
		SKIP_DOWNLOAD=1
	fi
fi

if [ $SKIP_DOWNLOAD -eq 1 ]; then
	echo "Skip download. Already file with proper checksum downloaded."
else
	# Remove old file (not correct checksum)
	wget --timestamping "$DOWNLOAD_URL"
	md5=($(md5sum "$DOWNLOAD_FILE"))
	if [ "$md5" = "$MD5" ]; then
		echo "Checksum incorrect. Remove downloaded file and exit"
		#rm -f $DOWNLOAD_FILE
		exit 1
	fi
	unzip -n $DOWNLOAD_FILE
fi

echo "Install $ZIP_FOLDER/$DEB_FILE file using apt (sudo rights required)"
sudo apt install "$ZIP_FOLDER/$DEB_FILE"
