#!/bin/bash

SERIAL_NUM=$1

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh
source $path/_config.sh

$path/_jlink.sh $JLINK_SCRIPT_DIR/erase.script $SERIAL_NUM
