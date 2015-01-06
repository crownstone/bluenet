#!/bin/bash

PROGRAM=crownstone.hex
ADDRESS=F2:E1:DE:0D:92:30

path=$(pwd)
cd /opt/nrf51_dfu_linux

file=$path/../build/$PROGRAM
python dfu.py -f $file -a $ADDRESS
