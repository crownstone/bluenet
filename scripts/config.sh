#!/bin/sh

if [ -z $BLUENET_DIR ]; then
	echo "ERROR: environment variable 'BLUENET_DIR' should be set."
	exit 1
fi

source ${BLUENET_DIR}/CMakeBuild.config
