#!/bin/sh

if [ $# -ne 4 ]; then
	echo "Error: Usage $0 \"softdevice dir\" \"softdevice\" \"compiler dir\" \"compiler type\""
	exit
fi
SOFTDEVICE_DIR=$1
SOFTDEVICE=$2
COMPILER_PATH=$3
COMPILER_TYPE=$4

FULLNAME_SOFTDEVICE=${SOFTDEVICE_DIR}/${SOFTDEVICE}_softdevice.hex

if [ ! -e ${FULLNAME_SOFTDEVICE} ]; then
	echo "Error: ${FULLNAME_SOFTDEVICE} does not exist..."
	exit
fi

# Output configuration file
NRF_UICR_BIN=${SOFTDEVICE_DIR}/softdevice_uicr.bin
# Output main file
NRF_MAIN_BIN=${SOFTDEVICE_DIR}/softdevice_mainpart.bin

OBJCOPY=${COMPILER_PATH}/bin/${COMPILER_TYPE}-objcopy

$OBJCOPY -Iihex -Obinary --only-section .sec3 ${FULLNAME_SOFTDEVICE} ${NRF_UICR_BIN}
$OBJCOPY -Iihex -Obinary --remove-section .sec3 ${FULLNAME_SOFTDEVICE} ${NRF_MAIN_BIN}

echo "Created files:"
echo "${NRF_UICR_BIN}"
echo "${NRF_MAIN_BIN}"
