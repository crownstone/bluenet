#!/bin/bash

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "$path/_utils.sh"

log_config() {
	cs_info "[_config.sh] BLUENET_WORKSPACE_DIR is ${BLUENET_WORKSPACE_DIR}"
	cs_info "[_config.sh] BLUENET_DIR is ${BLUENET_DIR}"
	cs_info "[_config.sh] BLUENET_CONFIG_DIR is ${BLUENET_CONFIG_DIR}"
	cs_info "[_config.sh] BLUENET_BUILD_DIR is ${BLUENET_BUILD_DIR}"
	cs_info "[_config.sh] BLUENET_BIN_DIR is ${BLUENET_BIN_DIR}"
	cs_info "[_config.sh] BLUENET_RELEASE_DIR is ${BLUENET_RELEASE_DIR}"
}

if [ ! -d "${BLUENET_DIR}" ] || [ -z "${BLUENET_CONFIG_DIR}" ] || [ -z "${BLUENET_BUILD_DIR}" ] || [ -z "${BLUENET_BIN_DIR}" ]; then
	log_config
	ls ${BLUENET_DIR}
	ls ${BLUENET_CONFIG_DIR}
	ls ${BLUENET_BUILD_DIR}
	ls ${BLUENET_BIN_DIR}
	cs_err "ERROR: missing environment variables, or wrongly set!!"
	cs_err " make sure to source the PATH/TO/YOUR/BLUENET_DIR/scripts/env.sh"
	cs_err " in your bashrc file with"
	cs_err "    $ echo \"source PATH/TO/YOUR/BLUENET_DIR/scripts/env.sh\" >> ~/.bashrc"
	cs_err " create a copy of the env.config"
	cs_err "    $ cp PATH/TO/YOUR/BLUENET_DIR/env.config.template PATH/TO/YOUR/BLUENET_DIR/env.config"
	cs_err " and define the environment variables correctly in"
	cs_err " PATH/TO/YOUR/BLUENET_DIR/env.config"
	env
	exit 1
fi

if [ -e ${BLUENET_DIR}/conf/cmake/CMakeBuild.config.default ]; then
	cs_log "source ${BLUENET_DIR}/conf/cmake/CMakeBuild.config.default"	
	source ${BLUENET_DIR}/conf/cmake/CMakeBuild.config.default
fi

if [ -e ${BLUENET_DIR}/CMakeBuild.config.local ]; then
	cs_log "source ${BLUENET_DIR}/CMakeBuild.config.local"
	source ${BLUENET_DIR}/CMakeBuild.config.local
fi

if [ ! -e ${BLUENET_CONFIG_DIR}/CMakeBuild.config ]; then
	cs_err "[_config.sh] ERROR: could not find ${BLUENET_CONFIG_DIR}/CMakeBuild.config"
	cs_err "[_config.sh] Don't forget to copy \"CMakeBuild.config.default\" to \"${BLUENET_CONFIG_DIR}/CMakeBuild.config\" and adjust the settings."
	exit 1
fi

source ${BLUENET_CONFIG_DIR}/CMakeBuild.config

## Use by default the softdevice folders from the SDK, if not otherwise specified
if [[ -z $SOFTDEVICE_DIR ]]; then
	SOFTDEVICE_DIR=$NRF5_DIR/components/softdevice/s$SOFTDEVICE_SERIES
fi

if [[ -z $SOFTDEVICE_DIR_API ]]; then
	SOFTDEVICE_DIR_API=headers
fi

if [[ -z $SOFTDEVICE_DIR_HEX ]]; then
	SOFTDEVICE_DIR_HEX=hex
fi

# Define jlink and temp script dir.
JLINK_SCRIPT_DIR="$path/jlink"
SCRIPT_TEMP_DIR="$path/tmp"
mkdir -p "$SCRIPT_TEMP_DIR"
