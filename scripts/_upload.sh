#!/bin/bash

FILE=${1:? "$0 requires \"file\" as argument"}
ADDRESS=${2:? "$0 requires \"address\" as argument"}
SERIAL_NUM=$3

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh
source $path/_config.sh

if [ ! -e ${FILE} ]; then
	cs_err "Error: ${FILE} does not exist..."
	exit $CS_ERR_FILE_NOT_FOUND
fi

#check_mime=$(mimetype $FILE)
#if [ $check_mime != "application/octet-stream" ]; then
#	cs_log "Error: ${FILE} has mime-type $check_mime instead of application/octet-stream"
#	cs_log "Are you perhaps trying to upload the .elf file?"
#	exit
#fi

if [[ $FILE != *.hex ]]; then
	cs_err "Error: ${FILE} has not the extension .hex"
	cs_err "Are you perhaps trying to upload the .elf file or the .bin file?"
	cs_err "Often a .bin file can be uploaded, but not if you have set configuration in other parts of memory"
	exit $CS_ERR_WRONG_FILE_TYPE
fi

cs_log "Write to address: $ADDRESS"

sed "s|@BIN@|$FILE|" $JLINK_SCRIPT_DIR/upload.script > $SCRIPT_TEMP_DIR/upload.script
sed -i "s|@START_ADDRESS@|$ADDRESS|" $SCRIPT_TEMP_DIR/upload.script

$path/jlink.sh $SCRIPT_TEMP_DIR/upload.script $SERIAL_NUM
