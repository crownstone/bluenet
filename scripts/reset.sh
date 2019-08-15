#!/bin/bash

# optional target, use crownstone as default
target=${1:-crownstone}

# Get the scripts path: the path where this file is located.
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

cs_info "Load configuration from: ${path}/_check_targets.sh $target"
source ${path}/_check_targets.sh $target

cs_info "Load configuration from: ${path}/_config.sh"
source $path/_config.sh

cs_info "resetting device"
if [ $serial_num ]; then
	nrfjprog -f nrf52 --reset --snr $serial_num
else
	nrfjprog -f nrf52 --reset
fi
