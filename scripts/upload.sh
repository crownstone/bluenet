#!/bin/bash

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/config.sh

SCRIPT_DIR=$path/jlink
TEMP_DIR=$path/tmp
mkdir -p $TEMP_DIR

DEVICE=nrf51822

FILE=${1:? "$0 requires \"file\" as first argument"}

ADDRESS=$2

if [ ! -e ${FILE} ]; then
	echo "Error: ${FILE} does not exist..."
	exit
fi

if [ -z "${ADDRESS}" ]; then
	echo "No address as parameter so overwrite with config value"
	ADDRESS=$APPLICATION_START_ADDRESS
fi
echo "Write to address: $ADDRESS"

#check_mime=$(mimetype $FILE)
#if [ $check_mime != "application/octet-stream" ]; then
#	echo "Error: ${FILE} has mime-type $check_mime instead of application/octet-stream"
#	echo "Are you perhaps trying to upload the .elf file?"
#	exit
#fi

if [[ $FILE != *.hex ]]; then
	echo "Error: ${FILE} has not the extension .hex"
	echo "Are you perhaps trying to upload the .elf file or the .bin file?"
	echo "Often a .bin file can be uploaded, but not if you have set configuration in other parts of memory"
	exit 1
fi

sed "s|@BIN@|$FILE|" $SCRIPT_DIR/upload.script > $TEMP_DIR/upload.script

echo "Program application starting from $ADDRESS"

sed -i "s|@START_ADDRESS@|$ADDRESS|" $TEMP_DIR/upload.script

if [ -z $3 ]; then
	$JLINK -Device $DEVICE -If SWD $TEMP_DIR/upload.script
else
	$JLINK -Device $DEVICE -SelectEmuBySN $3 -If SWD $TEMP_DIR/upload.script
fi
