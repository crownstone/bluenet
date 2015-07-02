#!/bin/sh

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_config.sh

SCRIPT_DIR=jlink

source _config.sh

DEVICE=nrf51822

SCRIPT=power.script

$JLINK -Device $DEVICE -If SWD $SCRIPT_DIR/$SCRIPT
