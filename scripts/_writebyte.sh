#!/bin/bash

ADDRESS=${1:? "$0 requires address as argument"}
VALUE=${2:? "$0 requires value as argument"}
SERIAL_NUM=$3

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh
source $path/_config.sh

sed "s|@ADDRESS@|$ADDRESS|" $JLINK_SCRIPT_DIR/writebyte.script > $SCRIPT_TEMP_DIR/writebyte.script.1
sed "s|@VALUE@|$VALUE|" $SCRIPT_TEMP_DIR/writebyte.script.1 > $SCRIPT_TEMP_DIR/writebyte.script

$path/_jlink.sh $SCRIPT_TEMP_DIR/writebyte.script $SERIAL_NUM
