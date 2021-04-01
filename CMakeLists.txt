cmake_minimum_required(VERSION 3.10)

#######################################################################################################################
# Set board target
#######################################################################################################################

option(DOWNLOAD_JLINK                            "Download JLink"                        ON)
option(DOWNLOAD_NRFUTIL                          "Download nrfutil"                      ON)
option(DOWNLOAD_NRFJPROG                         "Download nrfjprog"                     ON)
option(DOWNLOAD_NRFCONNECT                       "Download nrfconnect"                   OFF)
option(DOWNLOAD_NRFCONNECT_PROGRAMMER            "Download nrfconnect programmer"        OFF)
option(DOWNLOAD_CSUTIL                           "Download csutil utility"               OFF)
option(DOWNLOAD_BLUENET_LIB_LOGS                 "Download lib for binary logging"       OFF)

if(NOT CONFIG_DIR)
	set(CONFIG_DIR "config")
endif()

if(NOT BOARD_TARGET)
	set(BOARD_TARGET "default")
endif()

set(WORKSPACE_DIR                                ${CMAKE_CURRENT_LIST_DIR})
set(DEFAULT_CONF_CMAKE_PATH                      ${WORKSPACE_DIR}/source/conf/cmake)
set(DEFAULT_MODULES_PATH                         ${DEFAULT_CONF_CMAKE_PATH}/modules)

set(RELEASE_REPOSITORY                           ${WORKSPACE_DIR}/../bluenet-release CACHE STRING "Release repository")
set(RELEASE_CANDIDATE_REPOSITORY                 ${WORKSPACE_DIR}/../bluenet-release-candidate CACHE STRING "Release candidate repository")

# Set build type to debug, unless specified otherwise. This prevents accidental release builds.
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE                         Debug)
endif()

# Overwrite the typical install prefix
set(CMAKE_INSTALL_PREFIX                         ${CMAKE_SOURCE_DIR}/bin)

# With "make" sudo works more or less. Only when running make in parallel prompting is not always clear.
# With "ninja" sudo does not work. It might be worth looking into "pkexec" instead, but that requires more study.
set(SUPERUSER_SWITCH                             sudo    CACHE STRING "Switch to superuser")

# We are using python 3 by default, it's not a hard requirement though
set(PYTHON_EXECUTABLE                            python3 CACHE STRING "The python executable")

project(${BOARD_TARGET} NONE)

list(APPEND CMAKE_ARGS "-DWORKSPACE_DIR:STRING=${WORKSPACE_DIR}")
list(APPEND CMAKE_ARGS "-DRELEASE_REPOSITORY:STRING=${RELEASE_REPOSITORY}")
list(APPEND CMAKE_ARGS "-DRELEASE_CANDIDATE_REPOSITORY:STRING=${RELEASE_CANDIDATE_REPOSITORY}")
list(APPEND CMAKE_ARGS "-DCONFIG_DIR:STRING=${CONFIG_DIR}")
list(APPEND CMAKE_ARGS "-DBOARD_TARGET:STRING=${BOARD_TARGET}")
list(APPEND CMAKE_ARGS "-DDEFAULT_CONF_CMAKE_PATH:STRING=${DEFAULT_CONF_CMAKE_PATH}")
list(APPEND CMAKE_ARGS "-DDEFAULT_MODULES_PATH:STRING=${DEFAULT_MODULES_PATH}")
list(APPEND CMAKE_ARGS "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}")

#######################################################################################################################
# Load modules
#######################################################################################################################

message(STATUS "Assumed to be installed:")
message(STATUS "  git        - sudo apt-get install git")
message(STATUS "  python/pip - sudo apt-get install python python-pip")
message(STATUS "  wget       - sudo apt-get install wget")

message(STATUS "Optionally (for firmware):")
message(STATUS "  pass        - sudo apt-get install pass")
message(STATUS "  srec_cat    - sudo apt-get install srecord")

message(STATUS "Python set-up")
message(STATUS "  python executable: \"${PYTHON_EXECUTABLE}\"")
message(STATUS "  pip will be used as: \"${PYTHON_EXECUTABLE} -m pip install package\"")
message(STATUS "  if installation fails (of e.g. nrfutil), run \"python3 -m pip install -U pip [--user]\"")

# The source dir is the parent directory, the binary directory is the build/ directory (out of build)
message(STATUS "source dir: ${CMAKE_SOURCE_DIR}")
message(STATUS "bin dir: ${CMAKE_BINARY_DIR}")
message(STATUS "install dir: ${CMAKE_INSTALL_PREFIX}")

find_package(Git REQUIRED)

include(ExternalProject)

include(${DEFAULT_MODULES_PATH}/load_configuration.cmake)
include(${DEFAULT_MODULES_PATH}/git_info.cmake)
include(${DEFAULT_MODULES_PATH}/compilation_info.cmake)

#######################################################################################################################
# Load configuration values from multiple files
#######################################################################################################################

set(DEFAULT_CONFIGURATION_FILE ${DEFAULT_CONF_CMAKE_PATH}/CMakeBuild.config.default)
set(BOARD_TARGET_CONFIGURATION_FILE ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.config)
set(BOARD_TARGET_OVERWRITE_CONFIGURATION_FILE ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.overwrite.config)

load_configuration("${DEFAULT_CONFIGURATION_FILE}" CMAKE_ARGS)
load_configuration("${BOARD_TARGET_CONFIGURATION_FILE}" CMAKE_ARGS)
load_configuration("${BOARD_TARGET_OVERWRITE_CONFIGURATION_FILE}" CMAKE_ARGS)

#######################################################################################################################
# Download urls and hashes
#######################################################################################################################

# If you update the version of a file, also update the md5 hashh, e.g. through `md5sum downloads/*`

set(NORDIC_DOWNLOAD_URL https://www.nordicsemi.com/-/media/Software-and-other-downloads)

# The Nordic SDK
if (NORDIC_SDK_VERSION EQUAL 15)
	set(NORDIC_NRF5_SDK_DOWNLOAD_URL ${NORDIC_DOWNLOAD_URL}/SDKs/nRF5/Binaries/nRF5SDK153059ac345.zip)
	set(NORDIC_NRF5_SDK_MD5 46a8c9cd4b5d7ee4c5142e8fae3444c4)
elseif (NORDIC_SDK_VERSION EQUAL 16)
	set(NORDIC_NRF5_SDK_DOWNLOAD_URL ${NORDIC_DOWNLOAD_URL}/SDKs/nRF5/Binaries/nRF5SDK160098a08e2.zip)
	set(NORDIC_NRF5_SDK_MD5 cc2ccb57d2c7159fd37f9e04fca6cc64)
else()
	message(FATAL_ERROR "Unkown SDK version: ${NORDIC_SDK_VERSION}")
endif()


# The mesh
set(NORDIC_MESH_SDK_DOWNLOAD_URL ${NORDIC_DOWNLOAD_URL}/sDKS/nRF5-SDK-for-Mesh/nrf5SDKforMeshv320src.zip)
set(NORDIC_MESH_SDK_MD5 29e813a85e209fd07837c4fd3c616178)

# The nrfjprog
set(NRFJPROG_DOWNLOAD_URL ${NORDIC_DOWNLOAD_URL}/Desktop-software/nRF-command-line-tools/sw/Versions-10-x-x/10-12-1/nRFCommandLineTools10121Linuxamd64.tar.gz)
set(NRFJPROG_DEB_FILE nRF-Command-Line-Tools_10_12_1_Linux-amd64.deb)
set(NRFJPROG_MD5 a5d88595f13296614f07a8e8da6716b6)

# The cross-compiler
set(GCC_ARM_NONE_EABI_DOWNLOAD_URL https://developer.arm.com/-/media/Files/downloads/gnu-rm/8-2019q3/RC1.1/gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2?revision=c34d758a-be0c-476e-a2de-af8c6e16a8a2?product=GNU%20Arm%20Embedded%20Toolchain,64-bit,,Linux,8-2019-q3-update)
set(GCC_ARM_NONE_EABI_MD5 6341f11972dac8de185646d0fbd73bfc)

# The JLink tool
set(JLINK_DOWNLOAD_URL https://www.segger.com/downloads/jlink)
#set(JLINK_DEB_FILE JLink_Linux_V662d_x86_64.deb)
set(JLINK_DEB_FILE JLink_Linux_V650_x86_64.deb)
set(JLINK_LINUX_DOWNLOAD_URL ${JLINK_DOWNLOAD_URL}/${JLINK_DEB_FILE})
#set(JLINK_MD5 9da2ff634edbe0c8adf68bdb3c850b90)
set(JLINK_MD5 bdf926715a45dfd930d4516e6a5027c1)

#######################################################################################################################
# Local names
#######################################################################################################################


#set(SOFTDEVICE_HEX_FILE components/softdevice/s132/hex/s132_nrf52_6.1.1_softdevice.hex)
#set(SOFTDEVICE_HEX_FILE components/softdevice/s132/hex/s132_nrf52_7.0.1_softdevice.hex)
set(SOFTDEVICE_HEX_FILE components/softdevice/s132/hex/${SOFTDEVICE}_softdevice.hex)

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
	COMMAND cd ${WORKSPACE_DIR} && patch -p0 < patch/02nrf5.patch && patch -p0 < patch/03nrf5.patch
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

if(DOWNLOAD_CSUTIL)
	ExternalProject_Add(csutil
		GIT_REPOSITORY https://github.com/mrquincle/csutil
		SOURCE_DIR ${WORKSPACE_DIR}/tools/csutil
		BINARY_DIR ${WORKSPACE_DIR}/tools/csutil
		CONFIGURE_COMMAND ${CMAKE_COMMAND} -E echo "Skipping configure step."
		BUILD_COMMAND ${CMAKE_COMMAND} -E echo "Skipping build step."
		INSTALL_COMMAND ${CMAKE_COMMAND} -E echo "Skipping install step."
		)
endif()

if(DOWNLOAD_BLUENET_LIB_LOGS)
	ExternalProject_Add(bluenet_logs
		DOWNLOAD_COMMAND ${PYTHON_EXECUTABLE} -m pip install bluenet-logs
		DOWNLOAD_DIR ${WORKSPACE_DIR}/downloads
		SOURCE_DIR ${WORKSPACE_DIR}/tools/bluenet_logs
		UPDATE_COMMAND ${PYTHON_EXECUTABLE} -m pip install --upgrade bluenet-logs
		CONFIGURE_COMMAND ${CMAKE_COMMAND} -E echo "Skipping configure step."
		BUILD_COMMAND ${CMAKE_COMMAND} -E echo "Skipping build step."
		INSTALL_COMMAND ${CMAKE_COMMAND} -E echo "Skipping install step."
		)
endif()

if(FACTORY_IMAGE)
	MESSAGE(WARNING "Make a factory image. The usual CMakeBuild.config files will not be used. "
		"They will be overwritten by the CMakeFactoryImage.config files. "
		"Make sure that you set -DFACTORY_IMAGE=0 when you continue normal development. ")
	set(FACTORY_IMAGE_HARDWARE_CONFIGURATION_FILE ${WORKSPACE_DIR}/factory-images/hardware/${FACTORY_IMAGE_HARDWARE_CONFIG}/CMakeFactoryImage.config)
	set(FACTORY_IMAGE_SOFTWARE_CONFIGURATION_FILE ${WORKSPACE_DIR}/factory-images/software/${FACTORY_IMAGE_SOFTWARE_CONFIG}/CMakeFactoryImage.config)

	MESSAGE(STATUS "Load hardware configuration file for factory image at ${FACTORY_IMAGE_HARDWARE_CONFIGURATION_FILE}")
	load_configuration("${FACTORY_IMAGE_HARDWARE_CONFIGURATION_FILE}" CMAKE_ARGS)

	MESSAGE(STATUS "Load software configuration file for factory image at ${FACTORY_IMAGE_SOFTWARE_CONFIGURATION_FILE}")
	load_configuration("${FACTORY_IMAGE_SOFTWARE_CONFIGURATION_FILE}" CMAKE_ARGS)

	IF(EXISTS ${FACTORY_IMAGE_HARDWARE_CONFIGURATION_FILE})
		set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${FACTORY_IMAGE_HARDWARE_CONFIGURATION_FILE})
	ENDIF()
	IF(EXISTS ${FACTORY_IMAGE_SOFTWARE_CONFIGURATION_FILE})
		set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${FACTORY_IMAGE_SOFTWARE_CONFIGURATION_FILE})
	ENDIF()
endif()

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

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/${BOARD_TARGET}/stamp)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/${BOARD_TARGET}/tmp)

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

ADD_CUSTOM_TARGET(target_depend
	COMMAND cd ${BOARD_TARGET} && test -f "Makefile" && make depend || echo "-- First install"
	COMMENT "Run make depend in target directory"
	)

#######################################################################################################################
# Build bluenet as external project
#######################################################################################################################

if(NOT FACTORY_IMAGE)
	ExternalProject_Add(bluenet
		SOURCE_DIR ${WORKSPACE_DIR}/source
		BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${BOARD_TARGET}
		INSTALL_DIR ${WORKSPACE_DIR}/bin/${BOARD_TARGET}
		STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/${BOARD_TARGET}/stamp
		TMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/${BOARD_TARGET}/tmp
		CMAKE_CACHE_ARGS "${CMAKE_ARGS}"
		CMAKE_ARGS "${CMAKE_ARGS}"
		BUILD_ALWAYS 1
		DEPENDS micro_eec target_depend
		)
endif()

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.config)
if(EXISTS  ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.overwrite.config)
	set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.overwrite.config)
endif()

# Regretfully, this "trick" will make it impossible to pull after a change in CMakeBuild.config .
#ExternalProject_Add_Step(bluenet BUILD
#  COMMAND ${CMAKE_COMMAND} -E echo "-- Execute git update-index --skip-worktree for default config file to prevent accidental commits"
#  COMMAND git update-index --skip-worktree ${WORKSPACE_DIR}/config/default/CMakeBuild.config
#  )

if(NOT FACTORY_IMAGE)
	add_dependencies(bluenet gcc_arm_none_eabi)
	add_dependencies(bluenet nordic_sdk)
	add_dependencies(bluenet nordic_mesh_sdk)
endif()

#######################################################################################################################
# Write softdevice and read version of softdevice
#######################################################################################################################

SET(DEFAULT_TOOL_PARAM "-DDEFAULT_MODULES_PATH:STRING=${DEFAULT_MODULES_PATH}\
	-DDEFAULT_CONFIG_FILE:STRING=${DEFAULT_CONFIG_FILE}\
	-DCONFIG_FILE:STRING=${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.runtime.config\
	-DDEVICE:STRING=${DEVICE}\
	-DNRF_DEVICE_FAMILY:STRING=${NRF_DEVICE_FAMILY}\
	-DSERIAL_NUM:STRING=${SERIAL_NUM}\
	-DGDB_PORT:STRING=${GDB_PORT}\
	-DSWO_PORT:STRING=${SWO_PORT}\
	-DTELNET_PORT:STRING=${TELNET_PORT}\
	-DUART_DEVICE:STRING=${UART_DEVICE}\
	-DUART_BAUDRATE:STRING=${UART_BAUDRATE}\
	-DRTT_PORT:STRING=${RTT_PORT}")

ADD_CUSTOM_TARGET(write_softdevice
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=WRITE_BINARY" "-DBINARY=${NRF5_DIR}/${SOFTDEVICE_HEX_FILE}" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	#DEPENDS bluenet
	COMMENT "Flashes softdevice to remote target board"
	)

ADD_CUSTOM_TARGET(write_mbr_param_address
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=WRITE" "-DADDRESS=0x10001018" "-DVALUE=0x0007E000" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Writes MBR param page address to UICR"
	)

ADD_CUSTOM_TARGET(read_softdevice_version
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=READ" "-DADDRESS=0x000300C" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Reads version of softdevice"
	)

ADD_CUSTOM_TARGET(erase
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=ERASE" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Erase everything on target board"
	)

ADD_CUSTOM_TARGET(reset
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=RESET" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Reset target board"
	)

#######################################################################################################################
# Forward commands for a particular board towards its build directory
#######################################################################################################################

# Will not be called automatically (this is done through the external project bluenet)
ADD_CUSTOM_TARGET(${BOARD_TARGET}
	COMMAND make
	WORKING_DIRECTORY ${BOARD_TARGET}
	DEPENDS
	bluenet
	COMMENT "Build application"
	)

ADD_CUSTOM_TARGET(debug_application
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
# Create a new release
#######################################################################################################################

# Create a new version, create new release directory
# Copy CMakeBuild.config.release.default.release to this directory
# Check fields (e.g. compare with CMakeBuild.config.default and see if fields are missing)
# Check if we are on the master branch
# Check if we are on the latest public commit
# Check if we have non-committed changes
# Check if git version does not exist
# Create git commit with this release version
ADD_CUSTOM_TARGET(create_git_release
	COMMAND ${CMAKE_COMMAND} -DDEFAULT_MODULES_PATH=${DEFAULT_MODULES_PATH} -DWORKSPACE_DIR=${WORKSPACE_DIR} -P ${DEFAULT_MODULES_PATH}/release.cmake
	COMMENT "Create git release"
	)

ADD_CUSTOM_TARGET(create_git_release_bootloader
	COMMAND ${CMAKE_COMMAND} -DDEFAULT_MODULES_PATH=${DEFAULT_MODULES_PATH} -DWORKSPACE_DIR=${WORKSPACE_DIR} -P ${DEFAULT_MODULES_PATH}/release_bootloader.cmake
	COMMENT "Create git release for bootloader"
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
	WORKING_DIRECTORY ${WORKSPACE_DIR}/tools/nrfconnect_apps/pc-nrfconnect-programmer
	#DEPENDS git_nrfconnect_programmer
	COMMENT "Run nrf connect core"
	)



#######################################################################################################################
# Factory images
#######################################################################################################################

# We can not use math(EXPR ...) because it does not have support for unsigned integers and older cmake versions
# do not support hexadecimal values either
if(FACTORY_IMAGE)
	include(${DEFAULT_MODULES_PATH}/hex.cmake)
	include(${DEFAULT_MODULES_PATH}/load_hardware_version_mapping.cmake)

	message(STATUS "Get version for hardware \"${HARDWARE_BOARD}\"")
	load_hardware_version(${CMAKE_SOURCE_DIR}/source/include/cfg/cs_Boards.h ${HARDWARE_BOARD} LOAD_BOARD_HARDWARE_VERSION)
	message(STATUS "Got ${LOAD_BOARD_HARDWARE_VERSION}")
	to_hex_raw(${LOAD_BOARD_HARDWARE_VERSION} HARDWARE_BOARD_HEX 8 "0x")
	message(STATUS "Hexadecimal ${HARDWARE_BOARD_HEX}")

	if(NOT ${PRODUCT_FAMILY} STREQUAL "" AND NOT ${PRODUCT_MARKET} STREQUAL "" AND NOT ${PRODUCT_TYPE} STREQUAL "")
		from_hex(${PRODUCT_FAMILY} PRODUCT_FAMILY_DEC)
		from_hex(${PRODUCT_MARKET} PRODUCT_MARKET_DEC)
		from_hex(${PRODUCT_TYPE} PRODUCT_TYPE_DEC)
		to_hex_raw(${PRODUCT_FAMILY_DEC} PRODUCT_FAMILY_HEX 2 "")
		to_hex_raw(${PRODUCT_MARKET_DEC} PRODUCT_MARKET_HEX 2 "")
		to_hex_raw(${PRODUCT_TYPE_DEC} PRODUCT_TYPE_HEX 2 "")
		set(PRODUCT_FAMILY_TYPE "${PRODUCT_FAMILY_HEX}${PRODUCT_MARKET_HEX}${PRODUCT_TYPE_HEX}")
		fill(${PRODUCT_FAMILY_TYPE} PRODUCT_FAMILY_TYPE 8 "F" "0x")
	endif()

	if(NOT ${PRODUCT_MAJOR} STREQUAL "" AND NOT ${PRODUCT_MINOR} STREQUAL "" AND NOT ${PRODUCT_PATCH} STREQUAL "")
		from_hex(${PRODUCT_MAJOR} PRODUCT_MAJOR_DEC)
		from_hex(${PRODUCT_MINOR} PRODUCT_MINOR_DEC)
		from_hex(${PRODUCT_PATCH} PRODUCT_PATCH_DEC)
		to_hex_raw(${PRODUCT_MAJOR_DEC} PRODUCT_MAJOR_HEX 2 "")
		to_hex_raw(${PRODUCT_MINOR_DEC} PRODUCT_MINOR_HEX 2 "")
		to_hex_raw(${PRODUCT_PATCH_DEC} PRODUCT_PATCH_HEX 2 "")
		set(MAJOR_MINOR_PATCH "${PRODUCT_MAJOR_HEX}${PRODUCT_MINOR_HEX}${PRODUCT_PATCH_HEX}")
		fill(${MAJOR_MINOR_PATCH} MAJOR_MINOR_PATCH 8 "F" "0x")
	endif()

	if(NOT ${PRODUCTION_YEAR} STREQUAL "" AND NOT ${PRODUCTION_WEEK} STREQUAL "" AND NOT ${PRODUCT_HOUSING} STREQUAL "")
		from_hex(${PRODUCT_HOUSING} PRODUCT_HOUSING_DEC)
		to_hex_raw(${PRODUCT_HOUSING_DEC} PRODUCT_HOUSING_HEX 2 "")

		to_hex_raw(${PRODUCTION_YEAR} PRODUCTION_YEAR_HEX 2 "")
		to_hex_raw(${PRODUCTION_WEEK} PRODUCTION_WEEK_HEX 2 "")

		set(PRODUCTION_RUN_HOUSING "${PRODUCTION_YEAR_HEX}${PRODUCTION_WEEK_HEX}${PRODUCT_HOUSING_HEX}")
		fill(${PRODUCTION_RUN_HOUSING} PRODUCTION_RUN_HOUSING 8 "F" "0x")
	endif()

	set(SREC_CAT_UICR_BOOTLOADER        "-exclude;0x10001014;0x10001018;-generate;0x10001014;0x10001018;-l-e-constant;${UICR_BOOTLOADER_ADDRESS};4")
	set(SREC_CAT_MBR_SETTINGS           "-exclude;0x10001018;0x1000101C;-generate;0x10001018;0x1000101C;-l-e-constant;${MBR_SETTINGS};4")
	set(SREC_CAT_HARDWARE_BOARD         "-exclude;0x10001084;0x10001088;-generate;0x10001084;0x10001088;-l-e-constant;${HARDWARE_BOARD_HEX};4")
	set(SREC_CAT_PRODUCT_FAMILY_TYPE    "-exclude;0x10001088;0x1000108C;-generate;0x10001088;0x1000108C;-l-e-constant;${PRODUCT_FAMILY_TYPE};4")
	set(SREC_CAT_MAJOR_MINOR_PATCH      "-exclude;0x1000108C;0x10001090;-generate;0x1000108C;0x10001090;-l-e-constant;${MAJOR_MINOR_PATCH};4")
	set(SREC_CAT_PRODUCTION_RUN_HOUSING "-exclude;0x10001090;0x10001094;-generate;0x10001090;0x10001094;-l-e-constant;${PRODUCTION_RUN_HOUSING};4")

	string(TIMESTAMP FACTORY_IMAGE_TIMESTAMP "%Y-%U")

	fill(${FACTORY_IMAGE_SOFTWARE_VERSION} FACTORY_IMAGE_SOFTWARE_VERSION 4 "0" "")

	#	set(FACTORY_IMAGE_OUTPUT_FILE_NAME "${FACTORY_IMAGE_SOFTWARE_NAME}_${FACTORY_IMAGE_HARDWARE_NAME}_${FACTORY_IMAGE_TIMESTAMP}.hex")
	set(FACTORY_IMAGE_OUTPUT_FILE_NAME "${FACTORY_IMAGE_HARDWARE_CONFIG}/software_${FACTORY_IMAGE_SOFTWARE_VERSION}/factory-image.hex")
	#	set(FACTORY_IMAGE_OUTPUT_FILE_NAME "factory-image.hex")
	set(FACTORY_IMAGE_PATH_NAME factory-images/${FACTORY_IMAGE_HARDWARE_CONFIG}/software_${FACTORY_IMAGE_SOFTWARE_VERSION})

	# Add this target by default to make all when in factory image creation mode
	ADD_CUSTOM_TARGET(generate_factory_image ALL
		COMMAND ${CMAKE_COMMAND} -E echo
		COMMAND ${CMAKE_COMMAND} -E echo "** Create factory images"
		COMMAND ${CMAKE_COMMAND} -E echo "     MBR_SETTINGS: ${MBR_SETTINGS}"
		COMMAND ${CMAKE_COMMAND} -E echo "     UICR_BOOTLOADER_ADDRESS: ${UICR_BOOTLOADER_ADDRESS}"
		COMMAND ${CMAKE_COMMAND} -E echo "     BOARD_HARDWARE_VERSION: ${HARDWARE_BOARD_HEX}"
		COMMAND ${CMAKE_COMMAND} -E echo "     PRODUCT_FAMILY_TYPE: ${PRODUCT_FAMILY_TYPE}"
		COMMAND ${CMAKE_COMMAND} -E echo "     MAJOR_MINOR_PATCH: ${MAJOR_MINOR_PATCH}"
		COMMAND ${CMAKE_COMMAND} -E echo "     PRODUCTION_RUN_HOUSING: ${PRODUCTION_RUN_HOUSING}"
		COMMAND ${CMAKE_COMMAND} -E echo
		COMMAND ${CMAKE_COMMAND} -E echo "** Image to be created: ${FACTORY_IMAGE_OUTPUT_FILE_NAME}"
		COMMAND ${CMAKE_COMMAND} -E echo
		COMMAND test -z "${MBR_SETTINGS}" && echo "No MBR_SETTINGS set. Did you set all fields in the CMakeFactoryImage.config file?" && echo && exit 1 || exit 0
		COMMAND test -z "${UICR_BOOTLOADER_ADDRESS}" && echo "No UICR_BOOTLOADER_ADDRESS set. Did you set all fields set in the CMakeFactoryImage.config file?" && echo && exit 1 || exit 0
		COMMAND test -z "${HARDWARE_BOARD_HEX}" && echo "No HARDWARE_BOARD_HEX set. Did you set all fields set in the CMakeFactoryImage.config file?" && echo && exit 1 || exit 0
		COMMAND test -z "${PRODUCT_FAMILY_TYPE}" && echo "No PRODUCT_FAMILY_TYPE set. Did you set all fields set in the CMakeFactoryImage.config file?" && echo && exit 1 || exit 0
		COMMAND test -z "${MAJOR_MINOR_PATCH}" && echo "No MAJOR_MINOR_PATCH set. Did you set all fields set in the CMakeFactoryImage.config file?" && echo && exit 1 || exit 0
		COMMAND test -z "${PRODUCTION_RUN_HOUSING}" && echo "No PRODUCTION_RUN_HOUSING set. Did you set all fields set in the CMakeFactoryImage.config file?" && echo && exit 1 || exit 0
		COMMAND test -f "${FACTORY_IMAGE_APPLICATION_HEX_FILE}" && exit 0 || echo "Application missing. No such file: ${FACTORY_IMAGE_APPLICATION_HEX_FILE}" && echo && exit 1
		COMMAND test -f "${FACTORY_IMAGE_BOOTLOADER_HEX_FILE}" && exit 0 || echo "Bootloader missing. No such file: ${FACTORY_IMAGE_BOOTLOADER_HEX_FILE}" && echo && exit 1
		COMMAND test -f "${FACTORY_IMAGE_BOOTLOADER_SETTINGS_HEX_FILE}" && exit 0 || echo "Bootloader settings missing. No such file: ${FACTORY_IMAGE_BOOTLOADER_SETTINGS_HEX_FILE}" && echo && exit 1
		COMMAND test -f "${FACTORY_IMAGE_SOFTDEVICE_HEX_FILE}" && exit 0 || echo "Softdevice missing. No such file: ${FACTORY_IMAGE_SOFTDEVICE_HEX_FILE}" && echo && exit 1
		COMMAND ${CMAKE_COMMAND} -E echo "Use srec_cat to create a factory image"
		COMMAND ${CMAKE_COMMAND} -E make_directory ${FACTORY_IMAGE_PATH_NAME}
		COMMAND echo 'srec_cat ${FACTORY_IMAGE_APPLICATION_HEX_FILE} -intel ${SREC_CAT_UICR_BOOTLOADER} ${SREC_CAT_MBR_SETTINGS} ${SREC_CAT_HARDWARE_BOARD} ${SREC_CAT_PRODUCT_FAMILY_TYPE} ${SREC_CAT_MAJOR_MINOR_PATCH} ${SREC_CAT_PRODUCTION_RUN_HOUSING} ${FACTORY_IMAGE_BOOTLOADER_HEX_FILE} -intel ${FACTORY_IMAGE_BOOTLOADER_SETTINGS_HEX_FILE} -intel ${FACTORY_IMAGE_SOFTDEVICE_HEX_FILE} -intel -o factory-images/${FACTORY_IMAGE_OUTPUT_FILE_NAME} -intel'
		COMMAND srec_cat ${FACTORY_IMAGE_APPLICATION_HEX_FILE} -intel ${SREC_CAT_UICR_BOOTLOADER} ${SREC_CAT_MBR_SETTINGS} ${SREC_CAT_HARDWARE_BOARD} ${SREC_CAT_PRODUCT_FAMILY_TYPE} ${SREC_CAT_MAJOR_MINOR_PATCH} ${SREC_CAT_PRODUCTION_RUN_HOUSING} ${FACTORY_IMAGE_BOOTLOADER_HEX_FILE} -intel ${FACTORY_IMAGE_BOOTLOADER_SETTINGS_HEX_FILE} -intel ${FACTORY_IMAGE_SOFTDEVICE_HEX_FILE} -intel -o factory-images/${FACTORY_IMAGE_OUTPUT_FILE_NAME} -intel
		COMMENT "Generate a factory image"
		)

	INSTALL(DIRECTORY "${CMAKE_BINARY_DIR}/${FACTORY_IMAGE_PATH_NAME}/" DESTINATION "${FACTORY_IMAGE_PATH_NAME}" FILES_MATCHING PATTERN "*")
endif()



