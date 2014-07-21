#!/bin/sh

SCRIPT_DIR=jlink
mkdir -p tmp
TEMP_DIR=tmp

JLINK=/usr/bin/JLinkExe
DEVICE=nrf51822

FILE=${1:? "$0 requires \"file\" as first argument"}

if [ ! -e ${FILE} ]; then
	echo "Error: ${FILE} does not exist..."
	exit
fi

#check_mime=$(mimetype $FILE)
#if [ $check_mime != "application/octet-stream" ]; then
#	echo "Error: ${FILE} has mime-type $check_mime instead of application/octet-stream"
#	echo "Are you perhaps trying to upload the .elf file?"
#	exit
#fi

if [[ ! $FILE == *.bin ]]; then
	echo "Error: ${FILE} has not the extension .bin"
	echo "Are you perhaps trying to upload the .elf file?"
	exit
fi

sed "s|@BIN@|$FILE|" $SCRIPT_DIR/upload.script > $TEMP_DIR/upload.script
$JLINK -Device $DEVICE -If SWD $TEMP_DIR/upload.script 
