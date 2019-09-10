cmake_minimum_required(VERSION 3.10)

#######################################################################################################################
# Set board target 
#######################################################################################################################

option(DOWNLOAD_JLINK                            "Download JLink"              ON)
option(DOWNLOAD_NRFUTIL                          "Download nrfutil"            ON)

set(BOARD_TARGET                                 default CACHE STRING "Board target to compile for")
set(WORKSPACE_DIR                                ${CMAKE_CURRENT_LIST_DIR})
set(DEFAULT_CONF_CMAKE_PATH                      ${WORKSPACE_DIR}/source/conf/cmake)
set(DEFAULT_MODULES_PATH                         ${DEFAULT_CONF_CMAKE_PATH}/modules)

project(${BOARD_TARGET})

list(APPEND CMAKE_ARGS "-DWORKSPACE_DIR:STRING=${WORKSPACE_DIR}")
list(APPEND CMAKE_ARGS "-DBOARD_TARGET:STRING=${BOARD_TARGET}")
list(APPEND CMAKE_ARGS "-DDEFAULT_CONF_CMAKE_PATH:STRING=${DEFAULT_CONF_CMAKE_PATH}")
list(APPEND CMAKE_ARGS "-DDEFAULT_MODULES_PATH:STRING=${DEFAULT_MODULES_PATH}")

#######################################################################################################################
# Load modules
#######################################################################################################################

find_package(Git REQUIRED)

include(ExternalProject)

include(${DEFAULT_MODULES_PATH}/load_configuration.cmake)
include(${DEFAULT_MODULES_PATH}/git_info.cmake)
include(${DEFAULT_MODULES_PATH}/compilation_info.cmake)

#######################################################################################################################
# Download dependencies
#######################################################################################################################

set(NORDIC_DOWNLOAD_URL https://www.nordicsemi.com/-/media/Software-and-other-downloads/SDKs)

# If files are updated... Also update the md5 hash, e.g. through `md5sum downloads/*`
set(NORDIC_NRF5_SDK_DOWNLOAD_URL ${NORDIC_DOWNLOAD_URL}/nRF5/Binaries/nRF5SDK153059ac345.zip)
set(NORDIC_NRF5_SDK_MD5 46a8c9cd4b5d7ee4c5142e8fae3444c4)
set(NORDIC_MESH_SDK_DOWNLOAD_URL ${NORDIC_DOWNLOAD_URL}/nRF5-SDK-for-Mesh/nrf5SDKforMeshv320src.zip)
set(NORDIC_MESH_SDK_MD5 29e813a85e209fd07837c4fd3c616178)

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

  set(JLINK_DOWNLOAD_URL https://www.segger.com/downloads/jlink)
  set(JLINK_DEB_FILE JLink_Linux_V650_x86_64.deb)
  set(JLINK_LINUX_DOWNLOAD_URL ${JLINK_DOWNLOAD_URL}/${JLINK_DEB_FILE})
  set(JLINK_MD5 bdf926715a45dfd930d4516e6a5027c1)

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

set(GCC_ARM_NONE_EABI_DOWNLOAD_URL https://developer.arm.com/-/media/Files/downloads/gnu-rm/8-2019q3/RC1.1/gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2?revision=c34d758a-be0c-476e-a2de-af8c6e16a8a2?product=GNU%20Arm%20Embedded%20Toolchain,64-bit,,Linux,8-2019-q3-update)
set(GCC_ARM_NONE_EABI_MD5 6341f11972dac8de185646d0fbd73bfc)

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
  set(NRFUTIL_DOWNLOAD_URL https://www.nordicsemi.com/-/media/Software-and-other-downloads/Desktop-software/nRF-command-line-tools/sw/Versions-10-x-x/nRFCommandLineTools1030Linuxamd64tar.gz)
  set(NRFUTIL_DEB_FILE nRF-Command-Line-Tools_10_3_0_Linux-amd64.deb)
  set(NRFUTIL_MD5 545df3556fc1a70da7a99926fb426eb3)

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
#list(APPEND CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${CMAKE_CONF_CMAKE_PATH}/arm.toolchain.cmake")

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
# Build bluenet as external project
#######################################################################################################################

ADD_CUSTOM_TARGET(micro_eec
  COMMAND chmod u+x build_all.sh WORKING_DIRECTORY ${NRF5_DIR}/external/micro-ecc
  COMMAND ${CMAKE_COMMAND} -E env "GNU_INSTALL_ROOT=${WORKSPACE_DIR}/tools/gcc_arm_none_eabi/bin/" ./build_all.sh WORKING_DIRECTORY ${NRF5_DIR}/external/micro-ecc
  COMMENT "Build micro-eec"
  DEPENDS 
    nordic_sdk
    gcc_arm_none_eabi
  )

ExternalProject_Add(bluenet
  SOURCE_DIR ${WORKSPACE_DIR}/source
  BINARY_DIR ${WORKSPACE_DIR}/build/${BOARD_TARGET}
  INSTALL_DIR ${WORKSPACE_DIR}/bin/${BOARD_TARGET}
  STAMP_DIR ${WORKSPACE_DIR}/stamp/${BOARD_TARGET}
  TMP_DIR ${WORKSPACE_DIR}/tmp/${BOARD_TARGET}
  CMAKE_CACHE_ARGS "${CMAKE_ARGS}"
  CMAKE_ARGS "${CMAKE_ARGS}"
  BUILD_ALWAYS 1
  DEPENDS micro_eec
  )

#list(APPEND CMAKE_BOOTLOADER_ARGS ${CMAKE_ARGS})
#list(APPEND CMAKE_BOOTLOADER_ARGS "-DBOOTLOADER_START_ADDRESS:STRING=${BOOTLOADER_START_ADDRESS}")
#list(APPEND CMAKE_BOOTLOADER_ARGS "-DBOOTLOADER_LENGTH:STRING=${BOOTLOADER_LENGTH}")
#list(APPEND CMAKE_BOOTLOADER_ARGS "-DBOOTLOADER_VERSION:STRING=${BOOTLOADER_VERSION}")
#list(APPEND CMAKE_BOOTLOADER_ARGS "-DBOARD_TARGET:STRING=${BOARD_TARGET}")

message(STATUS "GDB server port: ${GDB_PORT}")
#ExternalProject_Add(bootloader
#  SOURCE_DIR ${WORKSPACE_DIR}/source/bootloader
#  BINARY_DIR ${WORKSPACE_DIR}/build/bootloader/${BOARD_TARGET}
#  INSTALL_DIR ${WORKSPACE_DIR}/bin/${BOARD_TARGET}
#  STAMP_DIR ${WORKSPACE_DIR}/stamp/bootloader/${BOARD_TARGET}
#  TMP_DIR ${WORKSPACE_DIR}/tmp/bootloader/${BOARD_TARGET}
#  CMAKE_CACHE_ARGS "${CMAKE_BOOTLOADER_ARGS}"
#  CMAKE_ARGS "${CMAKE_BOOTLOADER_ARGS}"
#  BUILD_ALWAYS 1
#  DEPENDS micro_eec
#  )

add_dependencies(bluenet gcc_arm_none_eabi)
add_dependencies(bluenet nordic_sdk)
add_dependencies(bluenet nordic_mesh_sdk)
		
IF(SERIAL_NUM) 
  SET(SERIAL_NUM_SWITCH "--snr")
ENDIF()
		
ADD_CUSTOM_TARGET(softdevice
  COMMAND ${CMAKE_COMMAND} -E echo
  COMMAND ${CMAKE_COMMAND} -E echo "** Use nrfjprog to upload softdevice"
  COMMAND ${CMAKE_COMMAND} -E echo
  COMMAND nrfjprog -f nrf52 --program ${WORKSPACE_DIR}/tools/nrf5_sdk/components/softdevice/s132/hex/s132_nrf52_6.1.1_softdevice.hex --sectorerase ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}
  DEPENDS bluenet
  COMMENT "Flashes binary to remote target board"
  )

ADD_CUSTOM_TARGET(read_softdevice_version
  COMMAND ${CMAKE_COMMAND} -E echo
  COMMAND ${CMAKE_COMMAND} -E echo "** Use nrfjprog to get version of softdevice"
  COMMAND nrfjprog -f nrf52 --memrd 0x000300C ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}
  COMMAND ${CMAKE_COMMAND} -E echo
  COMMAND ${CMAKE_COMMAND} -E echo "** Version FFFF00A8 = S132 6.0.0"
  COMMAND ${CMAKE_COMMAND} -E echo "** Version FFFF00AF = S132 6.1.0"
  COMMAND ${CMAKE_COMMAND} -E echo "** Version FFFF00B7 = S132 6.1.1"
  COMMAND ${CMAKE_COMMAND} -E echo
  COMMENT "Reads version of softdevice"
  )

ADD_CUSTOM_TARGET(erase
  COMMAND ${CMAKE_COMMAND} -E echo
  COMMAND ${CMAKE_COMMAND} -E echo "** Use nrfjprog to erase target board"
  COMMAND ${CMAKE_COMMAND} -E echo
  COMMAND nrfjprog -f nrf52 --eraseall ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}
  COMMENT "Erase everything on target board"
  )

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

ADD_CUSTOM_TARGET(list_jlinks
  COMMAND JLinkExe -commanderscript list.jlink
  WORKING_DIRECTORY ${WORKSPACE_DIR}/source/conf/jlink
  DEPENDS 
    jlink
  COMMENT "Upload binary"
  )

ADD_CUSTOM_TARGET(upload_bootloader
  COMMAND make upload_bootloader 
  WORKING_DIRECTORY ${BOARD_TARGET}
  DEPENDS 
    bluenet
  COMMENT "Upload binary"
  )
