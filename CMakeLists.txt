cmake_minimum_required(VERSION 3.10)

#######################################################################################################################
# Set board target 
#######################################################################################################################

option(DOWNLOAD_JLINK                            "Download JLink"                        ON)
option(DOWNLOAD_NRFUTIL                          "Download nrfutil"                      ON)
option(DOWNLOAD_NRFCONNECT                       "Download nrfconnect"                   OFF)
option(DOWNLOAD_NRFCONNECT_PROGRAMMER            "Download nrfconnect programmer"        OFF)

set(BOARD_TARGET                                 default CACHE STRING "Board target to compile for")
set(WORKSPACE_DIR                                ${CMAKE_CURRENT_LIST_DIR})
set(DEFAULT_CONF_CMAKE_PATH                      ${WORKSPACE_DIR}/source/conf/cmake)
set(DEFAULT_MODULES_PATH                         ${DEFAULT_CONF_CMAKE_PATH}/modules)

set(CMAKE_BUILD_TYPE                             Debug   CACHE STRING "Default build type")

project(${BOARD_TARGET})

list(APPEND CMAKE_ARGS "-DWORKSPACE_DIR:STRING=${WORKSPACE_DIR}")
list(APPEND CMAKE_ARGS "-DBOARD_TARGET:STRING=${BOARD_TARGET}")
list(APPEND CMAKE_ARGS "-DDEFAULT_CONF_CMAKE_PATH:STRING=${DEFAULT_CONF_CMAKE_PATH}")
list(APPEND CMAKE_ARGS "-DDEFAULT_MODULES_PATH:STRING=${DEFAULT_MODULES_PATH}")
list(APPEND CMAKE_ARGS "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}")

#######################################################################################################################
# Load modules
#######################################################################################################################

find_package(Git REQUIRED)

include(ExternalProject)

include(${DEFAULT_MODULES_PATH}/load_configuration.cmake)
include(${DEFAULT_MODULES_PATH}/git_info.cmake)
include(${DEFAULT_MODULES_PATH}/compilation_info.cmake)

#######################################################################################################################
# Download urls and hashes
#######################################################################################################################

# If you update the version of a file, also update the md5 hashh, e.g. through `md5sum downloads/*`

set(NORDIC_DOWNLOAD_URL https://www.nordicsemi.com/-/media/Software-and-other-downloads)

# The Nordic SDK
set(NORDIC_NRF5_SDK_DOWNLOAD_URL ${NORDIC_DOWNLOAD_URL}/SDKs/nRF5/Binaries/nRF5SDK153059ac345.zip)
set(NORDIC_NRF5_SDK_MD5 46a8c9cd4b5d7ee4c5142e8fae3444c4)

# The mesh
set(NORDIC_MESH_SDK_DOWNLOAD_URL ${NORDIC_DOWNLOAD_URL}/sDKS/nRF5-SDK-for-Mesh/nrf5SDKforMeshv320src.zip)
set(NORDIC_MESH_SDK_MD5 29e813a85e209fd07837c4fd3c616178)

# The nrfutil
set(NRFUTIL_DOWNLOAD_URL ${NORDIC_DOWNLOAD_URL}/Desktop-software/nRF-command-line-tools/sw/Versions-10-x-x/nRFCommandLineTools1030Linuxamd64tar.gz)
set(NRFUTIL_DEB_FILE nRF-Command-Line-Tools_10_3_0_Linux-amd64.deb)
set(NRFUTIL_MD5 545df3556fc1a70da7a99926fb426eb3)

# The cross-compiler
set(GCC_ARM_NONE_EABI_DOWNLOAD_URL https://developer.arm.com/-/media/Files/downloads/gnu-rm/8-2019q3/RC1.1/gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2?revision=c34d758a-be0c-476e-a2de-af8c6e16a8a2?product=GNU%20Arm%20Embedded%20Toolchain,64-bit,,Linux,8-2019-q3-update)
set(GCC_ARM_NONE_EABI_MD5 6341f11972dac8de185646d0fbd73bfc)

# The JLink tool
set(JLINK_DOWNLOAD_URL https://www.segger.com/downloads/jlink)
set(JLINK_DEB_FILE JLink_Linux_V650_x86_64.deb)
set(JLINK_LINUX_DOWNLOAD_URL ${JLINK_DOWNLOAD_URL}/${JLINK_DEB_FILE})
set(JLINK_MD5 bdf926715a45dfd930d4516e6a5027c1)

#######################################################################################################################
# Local names
#######################################################################################################################

set(SOFTDEVICE_HEX_FILE components/softdevice/s132/hex/s132_nrf52_6.1.1_softdevice.hex)
list(APPEND CMAKE_ARGS "-DSOFTDEVICE_HEX_FILE:STRING=${SOFTDEVICE_HEX_FILE}")

#######################################################################################################################
# Download dependencies
#######################################################################################################################

ExternalProject_Add(nordic_sdk
  DOWNLOAD_DIR ${WORKSPACE_DIR}/downloads
  SOURCE_DIR ${WORKSPACE_DIR}/tools/nrf5_sdk
  URL ${NORDIC_NRF5_SDK_DOWNLOAD_URL}
  URL_MD5 ${NORDIC_NRF5_SDK_MD5}
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -E echo "Skipping configure step."
  BUILD_COMMAND ${CMAKE_COMMAND} -E echo "Skipping build step."
  INSTALL_COMMAND ${CMAKE_COMMAND} -E echo "Skipping install step."
  PATCH_COMMAND cd ${WORKSPACE_DIR} && patch -p0 < patch/01nrf5.patch
  )

set(NRF5_DIR ${WORKSPACE_DIR}/tools/nrf5_sdk)
list(APPEND CMAKE_ARGS "-DNRF5_DIR:STRING=${NRF5_DIR}")

ExternalProject_Add(nordic_mesh_sdk
  DOWNLOAD_DIR ${WORKSPACE_DIR}/downloads
  SOURCE_DIR ${WORKSPACE_DIR}/tools/mesh_sdk
  URL ${NORDIC_MESH_SDK_DOWNLOAD_URL}
  URL_MD5 ${NORDIC_MESH_SDK_MD5}
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -E echo "Skipping configure step."
  BUILD_COMMAND ${CMAKE_COMMAND} -E echo "Skipping build step."
  INSTALL_COMMAND ${CMAKE_COMMAND} -E echo "Skipping install step."
  PATCH_COMMAND cd ${WORKSPACE_DIR} && patch -p0 < patch/00mesh.patch
  )

set(MESH_SDK_DIR ${WORKSPACE_DIR}/tools/mesh_sdk)
list(APPEND CMAKE_ARGS "-DMESH_SDK_DIR:STRING=${MESH_SDK_DIR}")

if(DOWNLOAD_JLINK) 
  ExternalProject_Add(jlink
    DOWNLOAD_COMMAND wget  --post-data "accept_license_agreement=accepted" ${JLINK_LINUX_DOWNLOAD_URL} -O ${WORKSPACE_DIR}/downloads/${JLINK_DEB_FILE} 
    DOWNLOAD_DIR ${WORKSPACE_DIR}/downloads
    URL ${JLINK_LINUX_DOWNLOAD_URL}
    URL_MD5 ${JLINK_MD5}
    DOWNLOAD_NO_EXTRACT 1
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E echo "Skipping configure step."
    BUILD_COMMAND ${CMAKE_COMMAND} -E echo "Skipping build step."
    INSTALL_COMMAND sudo dpkg -i ${WORKSPACE_DIR}/downloads/${JLINK_DEB_FILE} 
    )
endif()

ExternalProject_Add(gcc_arm_none_eabi
  DOWNLOAD_DIR ${WORKSPACE_DIR}/downloads
  URL ${GCC_ARM_NONE_EABI_DOWNLOAD_URL}
  URL_MD5 ${GCC_ARM_NONE_EABI_MD5}
  SOURCE_DIR ${WORKSPACE_DIR}/tools/gcc_arm_none_eabi
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -E echo "Skipping configure step."
  BUILD_COMMAND ${CMAKE_COMMAND} -E echo "Skipping build step."
  INSTALL_COMMAND ${CMAKE_COMMAND} -E echo "Skipping install step."
  )

set(COMPILER_PATH ${WORKSPACE_DIR}/tools/gcc_arm_none_eabi)
list(APPEND CMAKE_ARGS "-DCOMPILER_PATH:STRING=${COMPILER_PATH}")

if(DOWNLOAD_NRFUTIL)
  ExternalProject_Add(nrfutil
    DOWNLOAD_DIR ${WORKSPACE_DIR}/downloads
    URL ${NRFUTIL_DOWNLOAD_URL}
    URL_MD5 ${NRFUTIL_MD5}
    SOURCE_DIR ${WORKSPACE_DIR}/tools/nrfutil
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E echo "Skipping configure step."
    BUILD_COMMAND ${CMAKE_COMMAND} -E echo "Skipping build step."
    INSTALL_COMMAND sudo dpkg -i ${WORKSPACE_DIR}/tools/nrfutil/${NRFUTIL_DEB_FILE} 
    )
endif()

if(DOWNLOAD_NRFCONNECT) 
  ExternalProject_Add(git_nrfconnect_core
    #GIT_REPOSITORY git@github.com:NordicSemiconductor/pc-nrfconnect-core
    GIT_REPOSITORY https://github.com/NordicSemiconductor/pc-nrfconnect-core
    SOURCE_DIR ${WORKSPACE_DIR}/tools/nrfconnect/pc-nrfconnect-core
    BINARY_DIR ${WORKSPACE_DIR}/tools/nrfconnect/pc-nrfconnect-core
    CONFIGURE_COMMAND sudo apt install build-essential python2.7 libudev-dev libgconf-2-4
    BUILD_COMMAND npm install
    INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory ${WORKSPACE_DIR}/tools/nrfconnect_apps 
            COMMAND ${CMAKE_COMMAND} -E create_symlink ${WORKSPACE_DIR}/tools/nrfconnect_apps $ENV{HOME}/.nrfconnect-apps/local
    )
endif()

if(DOWNLOAD_NRFCONNECT_PROGRAMMER) 
  ExternalProject_Add(git_nrfconnect_apps
    #GIT_REPOSITORY git@github.com:NordicSemiconductor/pc-nrfconnect-core
    GIT_REPOSITORY https://github.com/NordicSemiconductor/pc-nrfconnect-programmer
    SOURCE_DIR ${WORKSPACE_DIR}/tools/nrfconnect_apps/pc-nrfconnect-programmer
    BINARY_DIR ${WORKSPACE_DIR}/tools/nrfconnect_apps/pc-nrfconnect-programmer
    CONFIGURE_COMMAND sudo apt install build-essential python2.7 libudev-dev libgconf-2-4
    BUILD_COMMAND npm install
    INSTALL_COMMAND ${CMAKE_COMMAND} -E echo "Skipping install step."
    )
endif()

#######################################################################################################################
# Load configuration values from multiple files
#######################################################################################################################

set(DEFAULT_CONFIGURATION_FILE ${DEFAULT_CONF_CMAKE_PATH}/CMakeBuild.config.default)
set(BOARD_TARGET_CONFIGURATION_FILE ${WORKSPACE_DIR}/config/${BOARD_TARGET}/CMakeBuild.config)
set(BOARD_TARGET_OVERWRITE_CONFIGURATION_FILE ${WORKSPACE_DIR}/config/${BOARD_TARGET}/CMakeBuild.overwrite.config)

load_configuration("${DEFAULT_CONFIGURATION_FILE}" CMAKE_ARGS)
load_configuration("${BOARD_TARGET_CONFIGURATION_FILE}" CMAKE_ARGS)
load_configuration("${BOARD_TARGET_OVERWRITE_CONFIGURATION_FILE}" CMAKE_ARGS)

# For now only cross-compilation
list(APPEND CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${WORKSPACE_DIR}/source/arm.toolchain.cmake")

# Setting INSTALL_DIR is not sufficient, also CMAKE_INSTALL_PREFIX is required
list(APPEND CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX:FILEPATH=${WORKSPACE_DIR}/bin/${BOARD_TARGET}")

if(VERBOSITY GREATER 5)
  message(STATUS "list: ${CMAKE_ARGS}")
endif()

git_info("${WORKSPACE_DIR}/source" GIT_BRANCH GIT_HASH)

if(NOT GIT_BRANCH STREQUAL "")
  list(APPEND CMAKE_ARGS "-DGIT_BRANCH:STRING=${GIT_BRANCH}")
endif()

if(NOT GIT_HASH STREQUAL "")
  list(APPEND CMAKE_ARGS "-DGIT_HASH:STRING=${GIT_HASH}")
endif()

compilation_info(COMPILATION_DAY)

if(VERBOSITY GREATER 4)
  message(STATUS "git branch: ${GIT_BRANCH}")
  message(STATUS "git hash: ${GIT_HASH}")
  message(STATUS "compilation day: ${COMPILATION_DAY}")
endif()

if(NOT COMPILATION_DAY STREQUAL "")
  list(APPEND CMAKE_ARGS "-DCOMPILATION_DAY:STRING=${COMPILATION_DAY}")
endif()

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${WORKSPACE_DIR}/stamp/${BOARD_TARGET})
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${WORKSPACE_DIR}/tmp/${BOARD_TARGET})

#######################################################################################################################
# Build encryption library (required by bootloader)
#######################################################################################################################

ADD_CUSTOM_TARGET(micro_eec
  COMMAND chmod u+x build_all.sh WORKING_DIRECTORY ${NRF5_DIR}/external/micro-ecc
  COMMAND ${CMAKE_COMMAND} -E env "GNU_INSTALL_ROOT=${WORKSPACE_DIR}/tools/gcc_arm_none_eabi/bin/" "MAKEFLAGS=-j1" ./build_all.sh WORKING_DIRECTORY ${NRF5_DIR}/external/micro-ecc
  COMMENT "Build micro-eec"
  DEPENDS 
    nordic_sdk
    gcc_arm_none_eabi
  )

#######################################################################################################################
# Build mluenet as external project
#######################################################################################################################

ExternalProject_Add(bluenet
  SOURCE_DIR ${WORKSPACE_DIR}/source
  BINARY_DIR ${WORKSPACE_DIR}/build/${BOARD_TARGET}
  INSTALL_DIR ${WORKSPACE_DIR}/bin/${BOARD_TARGET}
  STAMP_DIR ${WORKSPACE_DIR}/build/${BOARD_TARGET}/stamp
  TMP_DIR ${WORKSPACE_DIR}/tmp/${BOARD_TARGET}
  CMAKE_CACHE_ARGS "${CMAKE_ARGS}"
  CMAKE_ARGS "${CMAKE_ARGS}"
  BUILD_ALWAYS 1
  DEPENDS micro_eec
  )

ExternalProject_Add_Step(bluenet BUILD
  COMMAND ${CMAKE_COMMAND} -E echo "-- Execute git update-index --skip-worktree for default config file to prevent accidental commits"
  COMMAND git update-index --skip-worktree ${WORKSPACE_DIR}/config/default/CMakeBuild.config
  )

add_dependencies(bluenet gcc_arm_none_eabi)
add_dependencies(bluenet nordic_sdk)
add_dependencies(bluenet nordic_mesh_sdk)

#######################################################################################################################
# Upload softdevice and read version of softdevice
#######################################################################################################################
		
IF(SERIAL_NUM) 
  set(SERIAL_NUM_SWITCH "--snr")
  set(SERIAL_NUM_DPARAM "-DSERIAL_NUM:STRING=${SERIAL_NUM}")
ENDIF()
		
ADD_CUSTOM_TARGET(softdevice
  COMMAND ${CMAKE_COMMAND} -E echo
  COMMAND ${CMAKE_COMMAND} -E echo "** Use nrfjprog to upload softdevice"
  COMMAND ${CMAKE_COMMAND} -E echo
  COMMAND nrfjprog -f nrf52 --program ${NRF5_DIR}/${SOFTDEVICE_HEX_FILE} --sectorerase ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}
  DEPENDS bluenet
  COMMENT "Flashes binary to remote target board"
  )
		
SET(DEFAULT_NRFUTIL_DPARAM "-DDEFAULT_MODULES_PATH:STRING=${DEFAULT_MODULES_PATH}")
SET(DEFAULT_NRFUTIL_READ_DPARAM "-DDEFAULT_MODULES_PATH:STRING=${DEFAULT_MODULES_PATH};-DINSTRUCTION:STRING=READ")

ADD_CUSTOM_TARGET(read_softdevice_version
  COMMAND ${CMAKE_COMMAND} ${DEFAULT_NRFUTIL_READ_DPARAM} -DADDRESS:STRING=0x000300C ${SERIAL_NUM_DPARAM} -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
  COMMENT "Reads version of softdevice"
  )

ADD_CUSTOM_TARGET(erase
  COMMAND ${CMAKE_COMMAND} -E echo
  COMMAND ${CMAKE_COMMAND} -E echo "** Use nrfjprog to erase target board"
  COMMAND ${CMAKE_COMMAND} -E echo
  COMMAND nrfjprog -f nrf52 --eraseall ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}
  COMMENT "Erase everything on target board"
  )

ADD_CUSTOM_TARGET(reset
  COMMAND ${CMAKE_COMMAND} -E echo
  COMMAND ${CMAKE_COMMAND} -E echo "** Use nrfjprog to reset target board"
  COMMAND ${CMAKE_COMMAND} -E echo
  COMMAND nrfjprog -f nrf52 --reset
  COMMENT "Reset target board"
  )

#######################################################################################################################
# Forward commands for a particular board towards its build directory 
#######################################################################################################################

ADD_CUSTOM_TARGET(${BOARD_TARGET}
  COMMAND make
  WORKING_DIRECTORY ${BOARD_TARGET}
  DEPENDS 
    bluenet
  COMMENT "Build firmware"
  )

ADD_CUSTOM_TARGET(debug
  COMMAND cmake ..
  COMMAND make debug
  WORKING_DIRECTORY ${BOARD_TARGET}
  DEPENDS 
    bluenet
  COMMENT "Debug binary"
  )

ADD_CUSTOM_TARGET(write_hw_version
  COMMAND make write_hw_version
  WORKING_DIRECTORY ${BOARD_TARGET}
  DEPENDS 
    nrfutil
    bluenet
  COMMENT "Write hardware version"
  )

ADD_CUSTOM_TARGET(upload 
  COMMAND make upload 
  WORKING_DIRECTORY ${BOARD_TARGET}
  DEPENDS 
    nrfutil
    bluenet
  COMMENT "Upload binary"
  )

ADD_CUSTOM_TARGET(upload_bootloader
  COMMAND make upload_bootloader 
  WORKING_DIRECTORY ${BOARD_TARGET}
  DEPENDS 
    bluenet
  COMMENT "Upload binary"
  )

#######################################################################################################################
# Some more utility functions
#######################################################################################################################

ADD_CUSTOM_TARGET(list_jlinks
  COMMAND JLinkExe -commanderscript list.jlink
  WORKING_DIRECTORY ${WORKSPACE_DIR}/source/conf/jlink
  DEPENDS 
    jlink
  COMMENT "Upload binary"
  )

ADD_CUSTOM_TARGET(nrfconnect_core_setup
  COMMAND ${CMAKE_COMMAND} -E echo "Run in separate shell. This is an electron app, debug with Ctrl+Shift+I"
  COMMAND npm run dev
  WORKING_DIRECTORY ${WORKSPACE_DIR}/tools/nrfconnect/pc-nrfconnect-core
  #DEPENDS git_nrfconnect_core
  COMMENT "Run nrf connect corey"
  )

ADD_CUSTOM_TARGET(nrfconnect_core
  COMMAND ${CMAKE_COMMAND} -E echo "Run in separate shell. This is an electron app, debug with Ctrl+Shift+I"
  COMMAND npm run app
  WORKING_DIRECTORY ${WORKSPACE_DIR}/tools/nrfconnect/pc-nrfconnect-core
  #DEPENDS git_nrfconnect_core
  COMMENT "Run nrf connect corey"
  )

ADD_CUSTOM_TARGET(nrfconnect_programmer
  COMMAND npm run app
  WORKING_DIRECTORY ${WORKSPACE_DIR}/tools/nrfconnect/pc-nrfconnect-programmer
  #DEPENDS git_nrfconnect_programmer
  COMMENT "Run nrf connect corey"
  )
