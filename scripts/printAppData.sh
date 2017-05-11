#!/bin/bash

# Arguments to this script
#option=${1:? "Usage: $0 (version|power|reboot)"}

# Optionally target as second argument to the script
target=${1:-default}

# Load scripts from the current directory. The path is defined in this way so that it can also be called from 
# another directory (and not only from this directory).
#   _utils.sh has color information
#   _config.sh has information about BLUENET variables
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh
source ${path}/_check_targets.sh $target
source $path/_config.sh
SCRIPT_DIR=jlink
TEMP_DIR=$path/tmp
mkdir -p $TEMP_DIR

#memAddr=$(echo "obase=16;ibase=16;${BOOTLOADER_START_ADDRESS}-1000")
memAddr=$((${BOOTLOADER_START_ADDRESS}-0x5000))
echo $memAddr
memAddr=`printf "0x%x\n" $memAddr`
echo $memAddr

$path/_readbytes.sh $memAddr 0x5000

