#!/bin/bash

ADDRESS=${1:? "$0 requires address as argument"}
NUM_BYTES=${2:? "$0 requires number of bytes as argument"}
SERIAL_NUM=$3

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh
source $path/_config.sh

SCRIPT_DIR=$path/jlink
TEMP_DIR=$path/tmp
mkdir -p $TEMP_DIR

sed "s|@ADDRESS@|$ADDRESS|" $SCRIPT_DIR/readbytes.script > $TEMP_DIR/readbytes.script
sed -i "s|@NUM_BYTES@|$NUM_BYTES|" $TEMP_DIR/readbytes.script

$path/jlink.sh $TEMP_DIR/readbytes.script $SERIAL_NUM
