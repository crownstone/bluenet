#!/bin/sh

SCRIPT_DIR=jlink

JLINK=/usr/bin/JLinkExe
DEVICE=nrf51822

SCRIPT=power.script

$JLINK -Device $DEVICE -If SWD $SCRIPT_DIR/$SCRIPT
