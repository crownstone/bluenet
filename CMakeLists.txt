cmake_minimum_required(VERSION 3.10)

#######################################################################################################################
# Set board target 
#######################################################################################################################

option(DOWNLOAD_JLINK                            "Download JLink"                        ON)
option(DOWNLOAD_NRFUTIL                          "Download nrfutil"                      ON)
option(DOWNLOAD_NRFJPROG                         "Download nrfjprog"                     ON)
option(DOWNLOAD_NRFCONNECT                       "Download nrfconnect"                   OFF)
option(DOWNLOAD_NRFCONNECT_PROGRAMMER            "Download nrfconnect programmer"        OFF)

if(NOT CONFIG_DIR)
  set(CONFIG_DIR "config")
endif()

if(NOT BOARD_TARGET)
  set(BOARD_TARGET "default")
endif()

set(WORKSPACE_DIR                                ${CMAKE_CURRENT_LIST_DIR})
set(DEFAULT_CONF_CMAKE_PATH                      ${WORKSPACE_DIR}/source/conf/cmake)
set(DEFAULT_MODULES_PATH                         ${DEFAULT_CONF_CMAKE_PATH}/modules)

set(CMAKE_BUILD_TYPE                             Debug   CACHE STRING "Default build type")
set(SUPERUSER_SWITCH                             sudo    CACHE STRING "Switch to superuser")

set(PYTHON_EXECUTABLE                            python3 CACHE STRING "The python executable")

project(${BOARD_TARGET} NONE)

list(APPEND CMAKE_ARGS "-DWORKSPACE_DIR:STRING=${WORKSPACE_DIR}")
list(APPEND CMAKE_ARGS "-DCONFIG_DIR:STRING=${CONFIG_DIR}")
list(APPEND CMAKE_ARGS "-DBOARD_TARGET:STRING=${BOARD_TARGET}")
list(APPEND CMAKE_ARGS "-DDEFAULT_CONF_CMAKE_PATH:STRING=${DEFAULT_CONF_CMAKE_PATH}")
list(APPEND CMAKE_ARGS "-DDEFAULT_MODULES_PATH:STRING=${DEFAULT_MODULES_PATH}")
list(APPEND CMAKE_ARGS "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}")

#######################################################################################################################
# Load modules
#######################################################################################################################

message(STATUS "Assumed to be installed:")
message(STATUS "git  - sudo apt-get install git")
message(STATUS "python/pip - sudo apt-get install python python-pip")
message(STATUS "wget - sudo apt-get install wget")

message(STATUS "Using python version: ${PYTHON_EXECUTABLE}")

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

# The nrfjprog
set(NRFJPROG_DOWNLOAD_URL ${NORDIC_DOWNLOAD_URL}/Desktop-software/nRF-command-line-tools/sw/Versions-10-x-x/nRFCommandLineTools1030Linuxamd64tar.gz)
set(NRFJPROG_DEB_FILE nRF-Command-Line-Tools_10_3_0_Linux-amd64.deb)
set(NRFJPROG_MD5 545df3556fc1a70da7a99926fb426eb3)

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
        COMMAND cd ${WORKSPACE_DIR} && patch -p0 < patch/02nrf5.patch
  )

set(NRF5_DIR ${WORKSPACE_DIR}/tools/nrf5_sdk)
list(APPEND CMAKE_ARGS "-DNRF5_DIR:STRING=${NRF5_DIR}")

ExternalProject_Add(nordic_mesh_sdk
  #DOWNLOAD_DIR ${WORKSPACE_DIR}/downloads
  GIT_REPOSITORY https://github.com/crownstone/nRF5-SDK-for-Mesh
  SOURCE_DIR ${WORKSPACE_DIR}/tools/mesh_sdk
  #URL ${NORDIC_MESH_SDK_DOWNLOAD_URL}
  #URL_MD5 ${NORDIC_MESH_SDK_MD5}
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -E echo "Skipping configure step."
  BUILD_COMMAND ${CMAKE_COMMAND} -E echo "Skipping build step."
  INSTALL_COMMAND ${CMAKE_COMMAND} -E echo "Skipping install step."
  #PATCH_COMMAND cd ${WORKSPACE_DIR} && patch -p0 < patch/00mesh.patch
  )

set(MESH_SDK_DIR ${WORKSPACE_DIR}/tools/mesh_sdk)
list(APPEND CMAKE_ARGS "-DMESH_SDK_DIR:STRING=${MESH_SDK_DIR}")

if(DOWNLOAD_JLINK) 
  ExternalProject_Add(jlink
    DOWNLOAD_COMMAND wget --timestamping --post-data "accept_license_agreement=accepted&non_emb_ctr=confirmed" ${JLINK_LINUX_DOWNLOAD_URL}
    DOWNLOAD_DIR ${WORKSPACE_DIR}/downloads
    URL ${JLINK_LINUX_DOWNLOAD_URL}
    URL_MD5 ${JLINK_MD5}
    DOWNLOAD_NO_EXTRACT 1
    SOURCE_DIR ${WORKSPACE_DIR}/tools/jlink
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E echo "Skipping configure step."
    BUILD_COMMAND ${CMAKE_COMMAND} -E echo "Skipping build step."
    INSTALL_COMMAND ${SUPERUSER_SWITCH} dpkg -i ${WORKSPACE_DIR}/downloads/${JLINK_DEB_FILE} 
    # USES_TERMINAL_INSTALL 1 # Does not actually work
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

if(DOWNLOAD_NRFJPROG)
  ExternalProject_Add(nrfjprog
    DOWNLOAD_DIR ${WORKSPACE_DIR}/downloads
    URL ${NRFJPROG_DOWNLOAD_URL}
    URL_MD5 ${NRFJPROG_MD5}
    SOURCE_DIR ${WORKSPACE_DIR}/tools/nrfjprog
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E echo "Skipping configure step."
    BUILD_COMMAND ${CMAKE_COMMAND} -E echo "Skipping build step."
    INSTALL_COMMAND ${SUPERUSER_SWITCH} dpkg -i ${WORKSPACE_DIR}/tools/nrfjprog/${NRFJPROG_DEB_FILE} 
    )
endif()

if(DOWNLOAD_NRFUTIL)
  ExternalProject_Add(nrfutil
    DOWNLOAD_COMMAND ${PYTHON_EXECUTABLE} -m pip install nrfutil
    DOWNLOAD_DIR ${WORKSPACE_DIR}/downloads
    SOURCE_DIR ${WORKSPACE_DIR}/tools/nrfutil
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E echo "Skipping configure step."
    BUILD_COMMAND ${CMAKE_COMMAND} -E echo "Skipping build step."
    INSTALL_COMMAND ${CMAKE_COMMAND} -E echo "Skipping install step."
    )
endif()

if(DOWNLOAD_NRFCONNECT) 
  ExternalProject_Add(git_nrfconnect_core
    GIT_REPOSITORY https://github.com/NordicSemiconductor/pc-nrfconnect-core
    SOURCE_DIR ${WORKSPACE_DIR}/tools/nrfconnect/pc-nrfconnect-core
    BINARY_DIR ${WORKSPACE_DIR}/tools/nrfconnect/pc-nrfconnect-core
    CONFIGURE_COMMAND ${SUPERUSER_SWITCH} apt-get install -y build-essential python2.7 libudev-dev libgconf-2-4
    BUILD_COMMAND npm install
    INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory ${WORKSPACE_DIR}/tools/nrfconnect_apps 
            COMMAND ${CMAKE_COMMAND} -E create_symlink ${WORKSPACE_DIR}/tools/nrfconnect_apps $ENV{HOME}/.nrfconnect-apps/local
    )
endif()

if(DOWNLOAD_NRFCONNECT_PROGRAMMER) 
  ExternalProject_Add(git_nrfconnect_apps
    GIT_REPOSITORY https://github.com/NordicSemiconductor/pc-nrfconnect-programmer
    SOURCE_DIR ${WORKSPACE_DIR}/tools/nrfconnect_apps/pc-nrfconnect-programmer
    BINARY_DIR ${WORKSPACE_DIR}/tools/nrfconnect_apps/pc-nrfconnect-programmer
    CONFIGURE_COMMAND ${SUPERUSER_SWITCH} apt-get install -y build-essential python2.7 libudev-dev libgconf-2-4
    BUILD_COMMAND npm install
    INSTALL_COMMAND ${CMAKE_COMMAND} -E echo "Skipping install step."
    )
endif()

#######################################################################################################################
# Load configuration values from multiple files
#######################################################################################################################

set(DEFAULT_CONFIGURATION_FILE ${DEFAULT_CONF_CMAKE_PATH}/CMakeBuild.config.default)
set(BOARD_TARGET_CONFIGURATION_FILE ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.config)
set(BOARD_TARGET_OVERWRITE_CONFIGURATION_FILE ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.overwrite.config)

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

#git_info("${WORKSPACE_DIR}/source" GIT_BRANCH GIT_HASH)
#if(NOT GIT_BRANCH STREQUAL "")
#  list(APPEND CMAKE_ARGS "-DGIT_BRANCH:STRING=${GIT_BRANCH}")
#endif()
#
#if(NOT GIT_HASH STREQUAL "")
#  list(APPEND CMAKE_ARGS "-DGIT_HASH:STRING=${GIT_HASH}")
#endif()
#
#compilation_info(COMPILATION_DAY)
#
#if(NOT COMPILATION_DAY STREQUAL "")
#  list(APPEND CMAKE_ARGS "-DCOMPILATION_DAY:STRING=${COMPILATION_DAY}")
#endif()

#message(STATUS "Git branch: ${GIT_BRANCH}")
#message(STATUS "Git hash: ${GIT_HASH}")
#message(STATUS "Compilation day: ${COMPILATION_DAY}")

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
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${BOARD_TARGET}
  INSTALL_DIR ${WORKSPACE_DIR}/bin/${BOARD_TARGET}
  STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/${BOARD_TARGET}/stamp
  TMP_DIR ${WORKSPACE_DIR}/tmp/${BOARD_TARGET}
  CMAKE_CACHE_ARGS "${CMAKE_ARGS}"
  CMAKE_ARGS "${CMAKE_ARGS}"
  BUILD_ALWAYS 1
  DEPENDS micro_eec
  )

		
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.config)
if(EXISTS  ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.overwrite.config)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.overwrite.config)
endif()

# Regretfully, this "trick" will make it impossible to pull after a change in CMakeBuild.config . 
#ExternalProject_Add_Step(bluenet BUILD
#  COMMAND ${CMAKE_COMMAND} -E echo "-- Execute git update-index --skip-worktree for default config file to prevent accidental commits"
#  COMMAND git update-index --skip-worktree ${WORKSPACE_DIR}/config/default/CMakeBuild.config
#  )

add_dependencies(bluenet gcc_arm_none_eabi)
add_dependencies(bluenet nordic_sdk)
add_dependencies(bluenet nordic_mesh_sdk)

#######################################################################################################################
# Write softdevice and read version of softdevice
#######################################################################################################################
		
IF(SERIAL_NUM) 
  set(SERIAL_NUM_SWITCH "--snr")
  set(SERIAL_NUM_DPARAM "-DSERIAL_NUM:STRING=${SERIAL_NUM}")
ENDIF()
		
ADD_CUSTOM_TARGET(write_softdevice
  COMMAND ${CMAKE_COMMAND} -E echo
  COMMAND ${CMAKE_COMMAND} -E echo "** Use nrfjprog to write softdevice"
  COMMAND ${CMAKE_COMMAND} -E echo
  COMMAND nrfjprog -f nrf52 --program ${NRF5_DIR}/${SOFTDEVICE_HEX_FILE} --sectorerase ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}
  DEPENDS bluenet
  COMMENT "Flashes softdevice to remote target board"
  )
		
SET(DEFAULT_NRFJPROG_DPARAM "-DDEFAULT_MODULES_PATH:STRING=${DEFAULT_MODULES_PATH}")
SET(DEFAULT_NRFJPROG_READ_DPARAM "-DDEFAULT_MODULES_PATH:STRING=${DEFAULT_MODULES_PATH};-DINSTRUCTION:STRING=READ")

ADD_CUSTOM_TARGET(read_softdevice_version
  COMMAND ${CMAKE_COMMAND} ${DEFAULT_NRFJPROG_READ_DPARAM} -DADDRESS:STRING=0x000300C ${SERIAL_NUM_DPARAM} -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
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
  COMMAND nrfjprog -f nrf52 --reset ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}
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
  COMMENT "Build application"
  )

ADD_CUSTOM_TARGET(debug_application
  COMMAND cmake ..
  COMMAND make debug_application
  WORKING_DIRECTORY ${BOARD_TARGET}
  DEPENDS 
    bluenet
  COMMENT "Debug application"
  )

ADD_CUSTOM_TARGET(write_board_version
  COMMAND make write_board_version
  WORKING_DIRECTORY ${BOARD_TARGET}
  DEPENDS 
    nrfjprog
    bluenet
  COMMENT "Write board version"
  )

ADD_CUSTOM_TARGET(write_application
  COMMAND make write_application
  WORKING_DIRECTORY ${BOARD_TARGET}
  DEPENDS
    nrfjprog
    bluenet
  COMMENT "Write application"
  )

ADD_CUSTOM_TARGET(write_bootloader
  COMMAND make write_bootloader
  WORKING_DIRECTORY ${BOARD_TARGET}
  DEPENDS
    bluenet
  COMMENT "Write bootloader"
  )

#######################################################################################################################
# Some more utility functions
#######################################################################################################################

ADD_CUSTOM_TARGET(list_jlinks
  COMMAND JLinkExe -commanderscript list.jlink
  WORKING_DIRECTORY ${WORKSPACE_DIR}/source/conf/jlink
  DEPENDS 
    jlink
  COMMENT "List JLink devices"
  )

ADD_CUSTOM_TARGET(nrfconnect_core_setup
  COMMAND ${CMAKE_COMMAND} -E echo "Run in separate shell. This is an electron app, debug with Ctrl+Shift+I"
  COMMAND npm run dev
  WORKING_DIRECTORY ${WORKSPACE_DIR}/tools/nrfconnect/pc-nrfconnect-core
  #DEPENDS git_nrfconnect_core
  COMMENT "Run nrf connect core"
  )

ADD_CUSTOM_TARGET(nrfconnect_core
  COMMAND ${CMAKE_COMMAND} -E echo "Run in separate shell. This is an electron app, debug with Ctrl+Shift+I"
  COMMAND npm run app
  WORKING_DIRECTORY ${WORKSPACE_DIR}/tools/nrfconnect/pc-nrfconnect-core
  #DEPENDS git_nrfconnect_core
  COMMENT "Run nrf connect core"
  )

ADD_CUSTOM_TARGET(nrfconnect_programmer
  COMMAND npm run app
  WORKING_DIRECTORY ${WORKSPACE_DIR}/tools/nrfconnect/pc-nrfconnect-programmer
  #DEPENDS git_nrfconnect_programmer
  COMMENT "Run nrf connect core"
  )
