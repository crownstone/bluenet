#!/bin/bash

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/config.sh

PROGRAM=crownstone.hex
ADDRESS=F2:E1:DE:0D:92:30

#TODO: make this configurable? - No, move this script to the dfu_linux repo
cd /opt/nrf51_dfu_linux

file=${BLUENET_DIR}/build/$PROGRAM
echo python dfu.py -f $file -a $ADDRESS
python dfu.py -f $file -a $ADDRESS
