#!/bin/bash

if [ $# -ne 6 ]; then
	echo "Error: Usage $0 \"softdevice dir\" \"softdevice bin dir\" \"softdevice\" \"compiler dir\" \"compiler type\" \"no separate section\""
	exit
fi
SOFTDEVICE_DIR=$1
SD_BINDIR=$2
SOFTDEVICE=$3
COMPILER_PATH=$4
COMPILER_TYPE=$5
SOFTDEVICE_NO_SEPARATE_UICR_SECTION=$6


FULLNAME_SOFTDEVICE=${SOFTDEVICE_DIR}/${SOFTDEVICE}_softdevice.hex

if [ ! -e ${FULLNAME_SOFTDEVICE} ]; then
	echo "Error: ${FULLNAME_SOFTDEVICE} does not exist..."
	exit
fi

mkdir -p $SD_BINDIR
# Output configuration file
NRF_UICR_BIN=${SD_BINDIR}/softdevice_uicr.bin
# Output main file
NRF_MAIN_BIN=${SD_BINDIR}/softdevice_mainpart.bin

# Remove old files
rm -f ${NRF_MAIN_BIN}
rm -f ${NRF_UICR_BIN}

OBJCOPY=${COMPILER_PATH}/bin/${COMPILER_TYPE}-objcopy


if [ $SOFTDEVICE_NO_SEPARATE_UICR_SECTION == 1 ]; then
	echo "SoftDevice has no separate UICR section"
	echo "$OBJCOPY -Iihex -Obinary ${FULLNAME_SOFTDEVICE} ${NRF_MAIN_BIN}"
	$OBJCOPY -Iihex -Obinary ${FULLNAME_SOFTDEVICE} ${NRF_MAIN_BIN}
else
	echo "Separately copy main and UICR section"
	echo "$OBJCOPY -Iihex -Obinary --only-section .sec3 ${FULLNAME_SOFTDEVICE} ${NRF_UICR_BIN}"
	$OBJCOPY -Iihex -Obinary --only-section .sec3 ${FULLNAME_SOFTDEVICE} ${NRF_UICR_BIN}
	echo "$OBJCOPY -Iihex -Obinary --remove-section .sec3 ${FULLNAME_SOFTDEVICE} ${NRF_MAIN_BIN}"
	$OBJCOPY -Iihex -Obinary --remove-section .sec3 ${FULLNAME_SOFTDEVICE} ${NRF_MAIN_BIN}
fi

echo "Created file(s):"
if [ ! $SOFTDEVICE_NO_SEPARATE_UICR_SECTION == 1 ]; then
	echo "  ${NRF_UICR_BIN}"
fi
echo "  ${NRF_MAIN_BIN}"
