#!/bin/bash

if [ $# -lt 0 ]; then
        echo "Usage: $0 [jlink-serial]"
        exit 1
fi

if [ $# -lt 1 ]; then
        nrfjprog --memrd 0x10001000 --n 0x100
        nrfjprog --reset
else
        nrfjprog --memrd 0x10001000 --n 0x100 -s $1
        nrfjprog --reset -s $1
fi

