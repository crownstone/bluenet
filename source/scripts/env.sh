#!/bin/bash

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# assign default BLUENET_DIR environment variable to the parent folder of the folder containing this script
if [ -z "${BLUENET_DIR}" ]; then
#	export BLUENET_DIR=$(readlink -m $( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/..)
	export BLUENET_DIR="$( cd "${path}/.." && pwd )"
fi

# load environment variables from env.config (if it exists)
if [ -e "${path}/../../config/env.config" ]; then
	set -o allexport
	source "${path}/../../config/env.config"
	set +o allexport
fi

# assign default paths to the environment variables if not defined otherwise
if [ ! -z "${BLUENET_WORKSPACE_DIR}" ]; then
	if [ -z "${BLUENET_DIR}" ]; then
		export BLUENET_DIR=${BLUENET_WORKSPACE_DIR}/source
	fi
	if [ -z "${BLUENET_CONFIG_DIR}" ]; then
		export BLUENET_CONFIG_DIR=${BLUENET_WORKSPACE_DIR}/config
	fi
	if [ -z "${BLUENET_BUILD_DIR}" ]; then
		export BLUENET_BUILD_DIR=${BLUENET_WORKSPACE_DIR}/build
	fi
	if [ -z "${BLUENET_BIN_DIR}" ]; then
		export BLUENET_BIN_DIR=${BLUENET_WORKSPACE_DIR}/bin
	fi
	if [ -z "${BLUENET_RELEASE_DIR}" ]; then
		export BLUENET_RELEASE_DIR=${BLUENET_WORKSPACE_DIR}/release
	fi
fi
