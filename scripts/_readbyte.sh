#!/bin/bash

ADDRESS=${1:? "$0 requires address as argument"}
SERIAL_NUM=$2

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh >/dev/null
source $path/_config.sh >/dev/null

sed "s|@ADDRESS@|$ADDRESS|" $JLINK_SCRIPT_DIR/readbyte.script > $SCRIPT_TEMP_DIR/readbyte.script

result0=$($path/jlink.sh $SCRIPT_TEMP_DIR/readbyte.script $SERIAL_NUM | grep -A1 identified | tail -n1)

if [ "$result0" == "" ]; then
	echo '<not found>'
fi

result1=$(echo "$result0" | cut -f2 -d'=' | sed 's/^ //g' | sed 's/ $//g' | tr -s ' ' ':')

echo "$result1" | awk -v FS=":" '{print $4,$3,$2,$1}' | tr -d ' ' | tr '[:upper:]' '[:lower:]'
