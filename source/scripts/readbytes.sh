#!/bin/sh

memory_address=${1:? "Requires memory address as argument (without 0x)"}
number_of_bytes=${2:? "Requires number of bytes as argument"}

./_readbytes.sh "0x$memory_address" $number_of_bytes | grep $memory_address | cut -f2 -d'=' | xargs | tac -s' ' | tr -d '\n' | tr -d ' '
