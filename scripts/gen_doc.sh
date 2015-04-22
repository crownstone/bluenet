#!/bin/bash

crnt_path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $crnt_path/config.sh

mkdir -p ../build/doc
cd ../build/doc

rel_path="../.."

dirs=( $(find $rel_path/include -maxdepth 1 -type d) )
dirs+=("$rel_path/include/third/nrf")
dirs+=("$rel_path/include/third/protocol")

dirs+=("${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_API}/include")
dirs+=("${NRF51822_DIR}/Include")
dirs+=("${NRF51822_DIR}/Include/sd_common")
dirs+=("${NRF51822_DIR}/Include/app_common")
dirs+=("${NRF51822_DIR}/Include/ble")
dirs+=("${NRF51822_DIR}/Include/ble/ble_services")
dirs+=("${NRF51822_DIR}/Include/gcc")
dirs+=("${NRF51822_DIR}/Include/s110")

#dirs+=("/usr/include/c++/4.9")
dirs+=("/opt/gcc-arm-none-eabi-4_8-2014q3/arm-none-eabi/include")
dirs+=("/opt/gcc-arm-none-eabi-4_8-2014q3/arm-none-eabi/include/c++/4.8.4")
dirs+=("/opt/gcc-arm-none-eabi-4_8-2014q3/arm-none-eabi/include/c++/4.8.4/arm-none-eabi")

# only include header files, source files are not considered
files=()
files+=("$rel_path/include/cs_nRF51822.h")
#files+=("$rel_path/include/cs_iBeacon.h") does not compile
files+=("$rel_path/include/cs_BluetoothLE.h")
files+=("$rel_path/include/common/cs_Boards.h")
files+=("$rel_path/include/common/cs_Config.h")
files+=("$rel_path/include/common/cs_Storage.h")
#files+=("$rel_path/include/common/cs_DifferentialBuffer.h") does not compile
files+=("$rel_path/include/common/cs_Timer.h")
files+=("$rel_path/include/services/cs_GeneralService.h")
files+=("$rel_path/include/services/cs_PowerService.h")
files+=("$rel_path/include/services/cs_IndoorLocalisationService.h")
files+=("$rel_path/include/characteristics/cs_ScanResult.h")
files+=("$rel_path/include/drivers/cs_PWM.h")
files+=("$rel_path/include/drivers/cs_ADC.h")
files+=("$rel_path/include/drivers/cs_Serial.h")
files=$(printf "%s " "${files[@]}")

# prefix every dir with "-I"
dirs=( "${dirs[@]/#/-I}" )

include_dirs=$(printf "%s " "${dirs[@]}")

#compiler_options="-O0 -g -nostdlib -ffreestanding -target arm-none-eabi -mcpu=cortex-m3 -mfloat-abi=soft -mthumb -std=c++11"
compiler_options="-std=c++11"

macros=()

macros+=("-MMD -DNRF51 -DEPD_ENABLE_EXTRA_RAM -DNRF51_USE_SOFTDEVICE=${NRF51_USE_SOFTDEVICE} -DUSE_RENDER_CONTEXT -DSYSCALLS -DUSING_FUNC")

# Pass variables in defined in the configuration file to the compiler
macros+=("-DNRF51822_DIR=${NRF51822_DIR}")
macros+=("-DNORDIC_SDK_VERSION=${NORDIC_SDK_VERSION}")
macros+=("-DSOFTDEVICE_SERIES=${SOFTDEVICE_SERIES}")
macros+=("-DSOFTDEVICE=${SOFTDEVICE}")
macros+=("-DSOFTDEVICE_NO_SEPARATE_UICR_SECTION=${SOFTDEVICE_NO_SEPARATE_UICR_SECTION}")
macros+=("-DAPPLICATION_START_ADDRESS=${APPLICATION_START_ADDRESS}")
macros+=("-DAPPLICATION_LENGTH=${APPLICATION_LENGTH}")
macros+=("-DCOMPILATION_TIME=${COMPILATION_TIME}")
macros+=("-DBOARD=${BOARD}")
macros+=("-DHARDWARE_VERSION=${HARDWARE_VERSION}")
macros+=("-DSERIAL_VERBOSITY=${SERIAL_VERBOSITY}")
macros+=("-DTICK_CONTINUOUSLY=${TICK_CONTINUOUSLY}")

# Add services
macros+=("-DINDOOR_SERVICE=${INDOOR_SERVICE}")
macros+=("-DGENERAL_SERVICE=${GENERAL_SERVICE}")
macros+=("-DPOWER_SERVICE=${POWER_SERVICE}")
macros+=("-DCHAR_MESHING=${CHAR_MESHING}")

# only required if Nordic files are used
macros+=("-DBOARD_NRF6310")

macros+=("-DBLUETOOTH_NAME=${BLUETOOTH_NAME}")

#macros+=("-D__MSVCRT__=1")
macro_defs=$(printf "%s " "${macros[@]}")

command="cldoc generate $include_dirs $compiler_options $macro_defs -- --output $rel_path/docs/html --report $files --basedir=$rel_path --merge=$rel_path/docs/source"
echo $command
$command
