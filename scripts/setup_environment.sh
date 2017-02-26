#!/bin/bash

source ./env.sh

target="test"

cfile="${BLUENET_CONFIG_DIR}/${target}"

echo 'BLUETOOTH_NAME="test"' >> $cfile
echo 'COMPILER_PATH=/usr' >> $cfile
echo 'COMPILER_TYPE=' >> $cfile
echo 'COMPILATION_TARGET=host' >> $cfile

cp _targets_template.sh ${BLUENET_CONFIG_DIR}/_targets.sh

env

