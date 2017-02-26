#!/bin/bash

script_path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

source $script_path/env.sh

target="test"

mkdir -p "${BLUENET_CONFIG_DIR}/${target}"
mkdir -p "${BLUENET_BUILD_DIR}/${target}"
mkdir -p "${BLUENET_BIN_DIR}/${target}"

mkdir -p "${BLUENET_RELEASE_DIR}"
mkdir -p "${BLUENET_BOOTLOADER_DIR}"

cfile="${BLUENET_CONFIG_DIR}/${target}/CMakeBuild.config"

echo 'BLUETOOTH_NAME="test"' >> $cfile
echo 'COMPILER_PATH=/usr' >> $cfile
echo 'COMPILER_TYPE=' >> $cfile
echo 'COMPILATION_TARGET=host' >> $cfile

cp $script_path/_targets_template.sh ${BLUENET_CONFIG_DIR}/_targets.sh

env

