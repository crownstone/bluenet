#!/bin/bash

SCRIPT_DIR=jlink
TEMP_DIR=tmp
mkdir -p $TEMP_DIR

ADDRESS=${1:? "Requires address as argument"}
VALUE=$2
DEVICE=nrf51822

source ../CMakeBuild.config

sed "s|@ADDRESS@|$ADDRESS|" $SCRIPT_DIR/writebyte.script > $TEMP_DIR/writebyte.script.1
sed "s|@VALUE@|$VALUE|" $TEMP_DIR/writebyte.script.1 > $TEMP_DIR/writebyte.script

echo "$JLINK -Device $DEVICE -If SWD $TEMP_DIR/writebyte.script "
$JLINK -Device $DEVICE -If SWD $TEMP_DIR/writebyte.script 

echo "The result should be something like:"
echo "    1000005C = 3C 00 FF FF"
