#!/bin/bash

ADDRESS=${1:? "$0 requires address as argument"}
NUM_BYTES=${2:? "$0 requires number of bytes as argument"}
SERIAL_NUM=$3

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh
source $path/_config.sh

sed "s|@ADDRESS@|$ADDRESS|" $JLINK_SCRIPT_DIR/readbytes.script > $SCRIPT_TEMP_DIR/readbytes.script
sed -i "s|@NUM_BYTES@|$NUM_BYTES|" $SCRIPT_TEMP_DIR/readbytes.script

$path/jlink.sh $SCRIPT_TEMP_DIR/readbytes.script $SERIAL_NUM
