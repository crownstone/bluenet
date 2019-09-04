#!/bin/bash

# Jlink script file.
jlink_script=${1:? "$0 requires \"jlink script file\" as first argument"}

# Optional target, use crownstone as default.
target=${2:-crownstone}

# Get the scripts path: the path where this file is located.
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

cs_info "Load configuration from: ${path}/_check_targets.sh $target"
source ${path}/_check_targets.sh $target

cs_info "Load configuration from: ${path}/_config.sh"
source $path/_config.sh

$path/_jlink.sh $jlink_script $serial_num
checkError "Error executing jlink script"
