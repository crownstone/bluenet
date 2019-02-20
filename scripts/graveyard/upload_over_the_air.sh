#!/bin/bash

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh
source $path/_config.sh

#PROGRAM=crownstone.hex
#ADDRESS=F2:E1:DE:0D:92:30

#PROGRAM=combined.hex
#ADDRESS=F4:60:EF:35:22:EC

PROGRAM=crownstone.hex
ADDRESS=F8:86:A5:56:42:51

#TODO: make this configurable? - No, move this script to the dfu_linux repo
cd /opt/nrf51_dfu_linux
cd $HOME/workspace/ble-automator

file=${BLUENET_BIN_DIR}/$PROGRAM
cs_log "python dfu.py -f $file -a $ADDRESS"
python dfu.py -f $file -a $ADDRESS
