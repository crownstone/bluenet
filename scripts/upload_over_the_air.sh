#!/bin/bash

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_config.sh

#PROGRAM=crownstone.hex
#ADDRESS=F2:E1:DE:0D:92:30

PROGRAM=combined.hex
ADDRESS=F4:60:EF:35:22:EC

#TODO: make this configurable? - No, move this script to the dfu_linux repo
cd /opt/nrf51_dfu_linux
cd $HOME/workspace/ble-automator

file=${BLUENET_BUILD_DIR}/$PROGRAM
echo python dfu.py -f $file -a $ADDRESS
python dfu.py -f $file -a $ADDRESS
