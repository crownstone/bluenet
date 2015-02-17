#!/bin/bash

#if [ -z $BLUENET_DIR ]; then
#	echo "ERROR: environment variable 'BLUENET_DIR' should be set."
#	exit 1
#fi

if [ ! -d "${BLUENET_DIR}" ]; then
	path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
	BLUENET_DIR=${path}/..
	echo "BLUENET_DIR does not exist. Use ${path}/.. as default"
fi

if [ ! -e ${BLUENET_DIR}/CMakeBuild.config ]; then
	echo "ERROR: could not find ${BLUENET_DIR}/CMakeBuild.config"
	echo "Don't forget to copy \"CMakeBuild.config.default\" to \"${BLUENET_DIR}/CMakeBuild.config\" and adjust the settings."
	exit 1
fi
source ${BLUENET_DIR}/CMakeBuild.config

