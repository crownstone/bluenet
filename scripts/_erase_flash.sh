#!/bin/bash

SERIAL_NUM=$1

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh
source $path/_config.sh

SCRIPT_DIR=$path/jlink
TEMP_DIR=$path/tmp
mkdir -p $TEMP_DIR

$path/jlink.sh $SCRIPT_DIR/erase.script $SERIAL_NUM
