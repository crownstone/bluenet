#!/bin/sh

SCRIPT_DIR=jlink

source config.sh

DEVICE=nrf51822

SCRIPT=power.script

$JLINK -Device $DEVICE -If SWD $SCRIPT_DIR/$SCRIPT
