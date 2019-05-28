#!/bin/bash

SERIAL_NUM=$1

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh
source $path/_config.sh

cs_info "resetting device"
if [ $SERIAL_NUM ]; then
	nrfjprog -f nrf52 --reset --snr $SERIAL_NUM
else
	nrfjprog -f nrf52 --reset
fi
