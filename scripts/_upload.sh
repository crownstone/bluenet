#!/bin/bash

FILE=${1:? "$0 requires \"file\" as first argument"}
ADDRESS=$2
SERIAL_NUM=$3

source _utils.sh

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_config.sh

SCRIPT_DIR=$path/jlink
TEMP_DIR=$path/tmp
mkdir -p $TEMP_DIR

#DEVICE=nrf51822
DEVICE=nRF52832_xxAA

if [ ! -e ${FILE} ]; then
	err "Error: ${FILE} does not exist..."
	exit 1
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
	err "Error: ${FILE} has not the extension .hex"
	err "Are you perhaps trying to upload the .elf file or the .bin file?"
	err "Often a .bin file can be uploaded, but not if you have set configuration in other parts of memory"
	exit 1
fi

sed "s|@BIN@|$FILE|" $SCRIPT_DIR/upload.script > $TEMP_DIR/upload.script

echo "Program application starting from $ADDRESS"

sed -i "s|@START_ADDRESS@|$ADDRESS|" $TEMP_DIR/upload.script

if [ -z $SERIAL_NUM ]; then
	echo "$JLINK -Device $DEVICE -speed 4000 -If SWD $TEMP_DIR/upload.script -ExitonError 1"
	$JLINK -Device $DEVICE -speed 4000 -If SWD $TEMP_DIR/upload.script -ExitonError 1
else
	echo "$JLINK -Device $DEVICE -speed 4000 -SelectEmuBySN $SERIAL_NUM -If SWD $TEMP_DIR/upload.script -ExitonError 1"
	$JLINK -Device $DEVICE -speed 4000 -SelectEmuBySN $SERIAL_NUM -If SWD $TEMP_DIR/upload.script -ExitonError 1
fi
