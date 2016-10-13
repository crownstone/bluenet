#!/bin/bash

#if [ -z $BLUENET_CONFIG_DIR ]; then
#	log "ERROR: environment variable 'BLUENET_CONFIG_DIR' should be set."
#	exit 1
#fi

# if [ ! -d "${BLUENET_DIR}" ]; then
	# config_path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
	# export BLUENET_DIR=${config_path}/..
	# log "BLUENET_DIR does not exist. Use ${config_path}/.. as default"
# fi

source "$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/_utils.sh"

log "BLUENET_WORKSPACE_DIR is ${BLUENET_WORKSPACE_DIR}"
log "BLUENET_DIR is ${BLUENET_DIR}"
log "BLUENET_CONFIG_DIR is ${BLUENET_CONFIG_DIR}"
log "BLUENET_BUILD_DIR is ${BLUENET_BUILD_DIR}"
log "BLUENET_BIN_DIR is ${BLUENET_BIN_DIR}"
log "BLUENET_RELEASE_DIR is ${BLUENET_RELEASE_DIR}"

if [ ! -d "${BLUENET_DIR}" ] || [ ! -d "${BLUENET_CONFIG_DIR}" ] || [ -z "${BLUENET_BUILD_DIR}" ] || [ -z "${BLUENET_BIN_DIR}" ]; then
	err "ERROR: missing environment variables, or wrongly set!!"
	err " make sure to source the PATH/TO/YOUR/BLUENET_DIR/scripts/env.sh"
	err " in your bashrc file with"
	err "    $ echo \"source PATH/TO/YOUR/BLUENET_DIR/scripts/env.sh\" >> ~/.bashrc"
	err " create a copy of the env.config"
	err "    $ cp PATH/TO/YOUR/BLUENET_DIR/env.config.template PATH/TO/YOUR/BLUENET_DIR/env.config"
	err " and define the environment variables correctly in"
	err " PATH/TO/YOUR/BLUENET_DIR/env.config"
	exit 1
fi

if [ -e ${BLUENET_DIR}/CMakeBuild.config.default ]; then
	source ${BLUENET_DIR}/CMakeBuild.config.default
fi

if [ -e ${BLUENET_DIR}/CMakeBuild.config.local ]; then
	source ${BLUENET_DIR}/CMakeBuild.config.local
fi

if [ ! -e ${BLUENET_CONFIG_DIR}/CMakeBuild.config ]; then
	err "ERROR: could not find ${BLUENET_CONFIG_DIR}/CMakeBuild.config"
	err "Don't forget to copy \"CMakeBuild.config.default\" to \"${BLUENET_CONFIG_DIR}/CMakeBuild.config\" and adjust the settings."
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
