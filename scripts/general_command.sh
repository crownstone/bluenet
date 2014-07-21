#!/bin/sh

SCRIPT_DIR=jlink

source ../CMakeBuild.config

DEVICE=nrf51822

SCRIPT=general.script

$JLINK -Device $DEVICE -If SWD $SCRIPT_DIR/$SCRIPT
