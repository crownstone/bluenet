#!/bin/bash

SCRIPT_DIR=jlink
TEMP_DIR=tmp
mkdir -p $TEMP_DIR

ADDRESS=${1:? "Requires address as argument"}
DEVICE=nrf51822

source config.sh

sed "s|@ADDRESS@|$ADDRESS|" $SCRIPT_DIR/readbyte.script > $TEMP_DIR/readbyte.script

echo "$JLINK -Device $DEVICE -If SWD $TEMP_DIR/readbyte.script"
$JLINK -Device $DEVICE -If SWD $TEMP_DIR/readbyte.script

echo "The result should be something like:"
echo "    1000005C = 3C 00 FF FF"
