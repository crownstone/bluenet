#!/bin/bash

ADDRESS=${1:? "$0 requires address as argument"}
VALUE=${2:? "$0 requires value as argument"}
SERIAL_NUM=$3

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh
source $path/_config.sh

#sed "s|@ADDRESS@|$ADDRESS|" $JLINK_SCRIPT_DIR/writebyte.script > $SCRIPT_TEMP_DIR/writebyte.script.1
#sed "s|@VALUE@|$VALUE|" $SCRIPT_TEMP_DIR/writebyte.script.1 > $SCRIPT_TEMP_DIR/writebyte.script
#$path/_jlink.sh $SCRIPT_TEMP_DIR/writebyte.script $SERIAL_NUM

current_value=$($path/_readbyte.sh $ADDRESS $SERIAL_NUM)
# remove the "0x"
current_value=${current_value:2}
# remove leading zeroes
current_value=$(echo "obase=16;ibase=16;$current_value+0" | bc )
current_value="0x$current_value"

new_value=$VALUE
# remove the "0x"
new_value=${new_value:2}
# change to upper case
new_value=$(echo $new_value | tr '[:lower:]' '[:upper:]')
# remove leading zeroes
new_value=$(echo "obase=16;ibase=16;$new_value+0" | bc )
# add the "0x" again
new_value="0x$new_value"


cs_info "Check value: $current_value vs $new_value"
if [ $current_value != $new_value ]; then
	cs_info "Write $new_value to $ADDRESS"
	if [ $SERIAL_NUM ]; then
		cs_info "nrfjprog -f nrf52 --memwr $ADDRESS --val $new_value --snr $SERIAL_NUM"
		nrfjprog -f nrf52 --memwr $ADDRESS --val $new_value --snr $SERIAL_NUM
	else
		cs_info "nrfjprog -f nrf52 --memwr $ADDRESS --val $new_value"
		nrfjprog -f nrf52 --memwr $ADDRESS --val $new_value
	fi
fi
