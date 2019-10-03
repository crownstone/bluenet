#!/bin/bash

# Don't use path here as variable name, as it will overwrite "path" of other scripts.
script_path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "$script_path/_utils.sh"

# Target if provided, otherwise default
target=${1:-default}

config_file=${BLUENET_SOURCE_DIR}/conf/cmake/CMakeBuild.config.default
if [ -f "$config_file" ]; then
	cs_info "source $config_file"	
	source "$config_file"
else
	cs_err "Missing config file: $config_file"
	exit 1
fi

config_file="${BLUENET_CONFIG_DIR}/${target}/CMakeBuild.config"
if [ -f "$config_file" ]; then
	cs_info "source $config_file"	
	source "$config_file"
else
	cs_err "Missing config file: $config_file"
	exit 1
fi

config_file="${BLUENET_CONFIG_DIR}/${target}/CMakeBuild.overwrite.config"
if [ -f "$config_file" ]; then
	cs_info "source $config_file"	
	source "$config_file"
fi
