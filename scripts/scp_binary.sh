#!/bin/bash

target=${1:-crownstone}

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh
source ${path}/_check_targets.sh $target
source $path/_config.sh

cs_log "Upload combined binary: ${BLUENET_BIN_DIR}/combined.hex"
cs_log "Done!"

rUser=pi
rIP=10.27.8.158
rFolder=/home/pi/dev/bluenet
rFile=MERGED_OUTPUT.hex

scp ${BLUENET_BIN_DIR}/combined.hex $rUser@$rIP:$rFolder/$rFile
