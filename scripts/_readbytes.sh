#!/bin/bash

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh
source $path/_config.sh

SCRIPT_DIR=$path/jlink
TEMP_DIR=$path/tmp
mkdir -p $TEMP_DIR

ADDRESS=${1:? "Requires address as argument"}
NUM_BYTES=${2:? "Requires number of bytes as argument"}
# DEVICE=nrf51822
DEVICE=nRF52832_xxAA

sed "s|@ADDRESS@|$ADDRESS|" $SCRIPT_DIR/readbytes.script > $TEMP_DIR/readbytes.script
sed -i "s|@NUM_BYTES@|$NUM_BYTES|" $TEMP_DIR/readbytes.script

echo "$JLINK -Device $DEVICE -speed 4000 -If SWD $TEMP_DIR/readbytes.script"
$JLINK -Device $DEVICE -speed 4000 -If SWD $TEMP_DIR/readbytes.script 
