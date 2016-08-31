#!/bin/bash

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_config.sh

#DEVICE=nrf51822

SCRIPT_DIR=jlink

SCRIPT=general.script

$JLINK -Device $DEVICE -If SWD $SCRIPT_DIR/$SCRIPT
