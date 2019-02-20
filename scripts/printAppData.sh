#!/bin/bash

# Optionally target as second argument to the script
target=${1:-crownstone}

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh
source ${path}/_check_targets.sh $target
source $path/_config.sh

#memAddr=$(echo "obase=16;ibase=16;${BOOTLOADER_START_ADDRESS}-1000")
memAddr=$((${BOOTLOADER_START_ADDRESS}-0x5000))
echo $memAddr
memAddr=`printf "0x%x\n" $memAddr`
echo $memAddr

$path/_readbytes.sh $memAddr 0x5000

