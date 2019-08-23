#!/bin/bash

SERIAL_NUM=$1

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh
source $path/_config.sh

#$path/_jlink.sh $JLINK_SCRIPT_DIR/erase.script $SERIAL_NUM

if [ $SERIAL_NUM ]; then
	cs_info "nrfjprog -f nrf52 --eraseall --snr $SERIAL_NUM"
	nrfjprog -f nrf52 --eraseall --snr $SERIAL_NUM
else
	cs_info "nrfjprog -f nrf52 --eraseall"
	nrfjprog -f nrf52 --eraseall
fi
