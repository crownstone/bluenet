#!/bin/sh

platform=${1:? "Usage: $0 <host|nrf>"}

this_script=$(readlink -f "$0")
this_script_path=$(dirname "$this_script")
cpd=$(echo "$this_script_path/..")

rm -f $cpd/CMakeLists.txt

if [ "$platform" = "host" ]; then
	ln -s $cpd/conf/cmake/CMakeLists.host_target.txt $cpd/CMakeLists.txt
fi

if [ "$platform" = "nrf" ]; then
	ln -s $cpd/conf/cmake/CMakeLists.nrf_target.txt $cpd/CMakeLists.txt
fi
