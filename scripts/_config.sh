#!/bin/bash

#if [ -z $BLUENET_CONFIG_DIR ]; then
#	echo "ERROR: environment variable 'BLUENET_CONFIG_DIR' should be set."
#	exit 1
#fi

# if [ ! -d "${BLUENET_DIR}" ]; then
	# config_path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
	# export BLUENET_DIR=${config_path}/..
	# echo "BLUENET_DIR does not exist. Use ${config_path}/.. as default"
# fi

echo "BLUENET_WORKSPACE_DIR is ${BLUENET_WORKSPACE_DIR}"
echo "BLUENET_DIR is ${BLUENET_DIR}"
echo "BLUENET_CONFIG_DIR is ${BLUENET_CONFIG_DIR}"
echo "BLUENET_BUILD_DIR is ${BLUENET_BUILD_DIR}"
echo "BLUENET_BIN_DIR is ${BLUENET_BIN_DIR}"
echo "BLUENET_RELEASE_DIR is ${BLUENET_RELEASE_DIR}"

if [ ! -d "${BLUENET_DIR}" ] || [ ! -d "${BLUENET_CONFIG_DIR}" ] || [ -z "${BLUENET_BUILD_DIR}" ] || [ -z "${BLUENET_BIN_DIR}" ]; then
	echo "ERROR: missing environment variables, or wrongly set!!"
	echo " make sure to source the \$BLUENET_DIR/scripts/env.sh"
	echo " in your bashrc file with"
	echo "    echo \"source \$BLUENET_DIR/scripts/env.sh\" >> ~/.bashrc"
	echo " and to define the environment variables correctly in"
	echo " \$BLUENET_DIR/env.config"
	exit 1
fi

if [ -e ${BLUENET_DIR}/CMakeBuild.config.default ]; then
	source ${BLUENET_DIR}/CMakeBuild.config.default
fi

if [ -e ${BLUENET_DIR}/CMakeBuild.config.local ]; then
	source ${BLUENET_DIR}/CMakeBuild.config.local
fi

if [ ! -e ${BLUENET_CONFIG_DIR}/CMakeBuild.config ]; then
	echo "ERROR: could not find ${BLUENET_CONFIG_DIR}/CMakeBuild.config"
	echo "Don't forget to copy \"CMakeBuild.config.default\" to \"${BLUENET_CONFIG_DIR}/CMakeBuild.config\" and adjust the settings."
	exit 1
fi

source ${BLUENET_CONFIG_DIR}/CMakeBuild.config

## Use by default the softdevice folders from the SDK, if not otherwise specified
if [[ -z $SOFTDEVICE_DIR ]]; then
	SOFTDEVICE_DIR=$NRF51822_DIR/components/softdevice/s$SOFTDEVICE_SERIES
fi

if [[ -z $SOFTDEVICE_DIR_API ]]; then
	SOFTDEVICE_DIR_API=headers
fi

if [[ -z $SOFTDEVICE_DIR_HEX ]]; then
	SOFTDEVICE_DIR_HEX=hex
fi
