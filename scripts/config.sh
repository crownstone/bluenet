#!/bin/sh

#if [ -z $BLUENET_DIR ]; then
#	echo "ERROR: environment variable 'BLUENET_DIR' should be set."
#	exit 1
#fi



if [ -e ${BLUENET_DIR}/CMakeBuild.config ]; then
	source ${BLUENET_DIR}/CMakeBuild.config
else
	path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
	if [ ! -e ${path}/../CMakeBuild.config ]; then
		echo "ERROR: could not find ${path}/../CMakeBuild.config"
		echo "Don't forget to copy \"CMakeBuild.config.default\" to \"CMakeBuild.config\" and adjust the settings."
		exit 1
	fi
	source ${path}/../CMakeBuild.config
fi


