#!/bin/bash

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh >/dev/null
source $path/_config.sh >/dev/null

SCRIPT_DIR=$path/jlink
TEMP_DIR=$path/tmp
mkdir -p $TEMP_DIR

ADDRESS=${1:? "Requires address as argument"}
# DEVICE=nrf51822
DEVICE=nRF52832_xxAA

sed "s|@ADDRESS@|$ADDRESS|" $SCRIPT_DIR/readbyte.script > $TEMP_DIR/readbyte.script

#echo "$JLINK -Device $DEVICE -speed 4000 -If SWD $TEMP_DIR/readbyte.script"
result0=$($JLINK -Device $DEVICE -speed 4000 -If SWD $TEMP_DIR/readbyte.script | grep -A1 identified | tail -n1)

if [ "$result0" == "" ]; then
	echo '<not found>'
fi

result1=$(echo "$result0" | cut -f2 -d'=' | sed 's/^ //g' | sed 's/ $//g' | tr -s ' ' ':')

echo "$result1" | awk -v FS=":" '{print $4,$3,$2,$1}' | tr -d ' ' | tr '[:upper:]' '[:lower:]'

