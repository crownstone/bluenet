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

config_path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ ! -d "${BLUENET_DIR}" ]; then

	if [ -d "${BLUENET_WORKSPACE_DIR}" ]; then
		BLUENET_DIR=${BLUENET_WORKSPACE_DIR}/bluenet
	else
		echo "ERROR: could not find environment variables BLUENET_DIR or BLUENET_WORKSPACE_DIR"
		echo "Need to define at least one of the two!!"
		# echo "ERROR: environment variable 'BLUENET_DIR' should be set."
		exit 1
	fi
fi

echo "BLUENET_DIR is ${BLUENET_DIR}"

if [ -e ${BLUENET_DIR}/CMakeBuild.config.default ]; then
	source ${BLUENET_DIR}/CMakeBuild.config.default
fi

if [ -e ${BLUENET_DIR}/CMakeBuild.config.local ]; then
	source ${BLUENET_DIR}/CMakeBuild.config.local
fi

if [ ! -d "${BLUENET_CONFIG_DIR}" ]; then

	if [ -d "${BLUENET_WORKSPACE_DIR}" ]; then
		BLUENET_CONFIG_DIR=${BLUENET_WORKSPACE_DIR}/config
	fi

	if [ ! -d "${BLUENET_CONFIG_DIR}" ]; then
		BLUENET_CONFIG_DIR=${config_path}/..
		echo "BLUENET_CONFIG_DIR does not exist. Use ${config_path}/.. as default"
	fi
fi
export BLUENET_CONFIG_DIR=${BLUENET_CONFIG_DIR}

echo "BLUENET_CONFIG_DIR is ${BLUENET_CONFIG_DIR}"

if [ -z "${BLUENET_BUILD_DIR}" ]; then

	if [ ! -z "${BLUENET_WORKSPACE_DIR}" ]; then
		BLUENET_BUILD_DIR=${BLUENET_WORKSPACE_DIR}/build
	else
		echo "ERROR: could not find BLUENET_BUILD_DIR or BLUENET_WORKSPACE_DIR"
		echo "Need to define at least one of the two!!"
		exit 1
	fi
fi
export BLUENET_BUILD_DIR=${BLUENET_BUILD_DIR}

echo "BLUENET_BUILD_DIR is ${BLUENET_BUILD_DIR}"

if [ -z "${BLUENET_BIN_DIR}" ]; then

	if [ ! -z "${BLUENET_WORKSPACE_DIR}" ]; then
		BLUENET_BIN_DIR=${BLUENET_WORKSPACE_DIR}/bin
	else
		echo "ERROR: could not find BLUENET_BIN_DIR or BLUENET_WORKSPACE_DIR"
		echo "Need to define at least one of the two!!"
		exit 1
	fi
fi
export BLUENET_BIN_DIR=${BLUENET_BIN_DIR}

echo "BLUENET_BIN_DIR is ${BLUENET_BIN_DIR}"

if [ -z "${BLUENET_RELEASE_DIR}" ]; then

	if [ ! -z "${BLUENET_WORKSPACE_DIR}" ]; then
		BLUENET_RELEASE_DIR=${BLUENET_WORKSPACE_DIR}/release
	else
		echo "ERROR: could not find BLUENET_RELEASE_DIR or BLUENET_WORKSPACE_DIR"
		echo "Need to define at least one of the two!!"
		exit 1
	fi
fi
export BLUENET_RELEASE_DIR=${BLUENET_RELEASE_DIR}

echo "BLUENET_RELEASE_DIR is ${BLUENET_RELEASE_DIR}"

# adjust targets and sets serial_num
# call it with the . so that it get's the same arguments as the call to this script
# and so that the variables assigned in the script will be persistent afterwards
. ${config_path}/_check_targets.sh

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
