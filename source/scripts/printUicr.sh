#!/bin/bash

# Optionally target as second argument to the script
target=${1:-crownstone}

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh
source ${path}/_check_targets.sh $target
source $path/_config.sh

echo "NRF uicr:"
$path/_readbytes.sh 0x10001000 0x40


echo "Hardware version:"
$path/_readbytes.sh $HARDWARE_BOARD_ADDRESS 0x40
