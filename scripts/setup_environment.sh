#!/bin/bash

script_path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

sudo add-apt-repository -y ppa:jonathonf/gcc-7.1
sudo apt update
sudo apt -y install gcc-7

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

echo "The gcc compilers"
gcc --version

echo "Show env. variables that contain the BLUENET string"
env | grep BLUENET
