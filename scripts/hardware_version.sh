#!/bin/bash

SCRIPT_DIR=jlink
TEMP_DIR=tmp
mkdir -p $TEMP_DIR

source config.sh
DEVICE=nrf51822

cp $SCRIPT_DIR/hardware_version.script $TEMP_DIR/hardware_version.script

echo "$JLINK -Device $DEVICE -If SWD $TEMP_DIR/hardware_version.script"
$JLINK -Device $DEVICE -If SWD $TEMP_DIR/hardware_version.script

echo "The result should be something like:"
echo "    1000005C = 3C 00 FF FF"
echo
echo "This means version 003C in the document:"
echo '    "Migrating to the latest nRF51822 chip version" "nWP-018" "White Paper v1.3"'
