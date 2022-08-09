message(STATUS "Version of cmake: ${CMAKE_VERSION}")

cmake_minimum_required(VERSION 3.10)

cmake_policy(SET CMP0135 NEW)

#######################################################################################################################
# Set board target
#######################################################################################################################

option(COMPILE_FOR_HOST                          "Compile for host"                      OFF)
option(FACTORY_IMAGE                             "Create factory image"                  OFF)
option(REQUIRE_NRFJPROG                          "Require nrfjprog"                      ON)
option(REQUIRE_JLINK                             "Require JLink"                         ON)
option(REQUIRE_PYTHON                            "Require python"                        OFF)

if(NOT CONFIG_DIR)
	set(CONFIG_DIR "config")
endif()

if(NOT BOARD_TARGET)
	set(BOARD_TARGET "default")
endif()

if($ENV{VERBOSE})
	set_property(GLOBAL PROPERTY GLOBAL_DEPENDS_DEBUG_MODE 1)
endif()

set(WORKSPACE_DIR                                ${CMAKE_CURRENT_LIST_DIR})
set(DEFAULT_CONF_CMAKE_PATH                      ${WORKSPACE_DIR}/source/conf/cmake)
set(DEFAULT_MODULES_PATH                         ${DEFAULT_CONF_CMAKE_PATH}/modules)

set(RELEASE_REPOSITORY                           ${WORKSPACE_DIR}/../bluenet-release CACHE STRING "Release repository")
set(RELEASE_CANDIDATE_REPOSITORY                 ${WORKSPACE_DIR}/../bluenet-release-candidate CACHE STRING "Release candidate repository")

set(OVERWRITE_COMPILER_VERSION                   "" CACHE STRING "Overwrite compiler version")

# Set build type to debug, unless specified otherwise. This prevents accidental release builds.
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE                         Debug)
endif()

# Overwrite the typical install prefix
set(CMAKE_INSTALL_PREFIX                         ${CMAKE_SOURCE_DIR}/bin)

# Create a more explicit name for this variable
set(ADD_TO_DEFAULT_BUILD_TARGET                  ALL)

# With "make" sudo works more or less. Only when running make in parallel prompting is not always clear.
# With "ninja" sudo does not work. It might be worth looking into "pkexec" instead, but that requires more study.
set(SUPERUSER_SWITCH                             sudo    CACHE STRING "Switch to superuser")

project(${BOARD_TARGET} NONE)

list(APPEND CMAKE_BLUENET_ARGS "-DWORKSPACE_DIR:STRING=${WORKSPACE_DIR}")
list(APPEND CMAKE_BLUENET_ARGS "-DRELEASE_REPOSITORY:STRING=${RELEASE_REPOSITORY}")
list(APPEND CMAKE_BLUENET_ARGS "-DRELEASE_CANDIDATE_REPOSITORY:STRING=${RELEASE_CANDIDATE_REPOSITORY}")
list(APPEND CMAKE_BLUENET_ARGS "-DCONFIG_DIR:STRING=${CONFIG_DIR}")
list(APPEND CMAKE_BLUENET_ARGS "-DBOARD_TARGET:STRING=${BOARD_TARGET}")
list(APPEND CMAKE_BLUENET_ARGS "-DDEFAULT_CONF_CMAKE_PATH:STRING=${DEFAULT_CONF_CMAKE_PATH}")
list(APPEND CMAKE_BLUENET_ARGS "-DDEFAULT_MODULES_PATH:STRING=${DEFAULT_MODULES_PATH}")
list(APPEND CMAKE_BLUENET_ARGS "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}")

#######################################################################################################################
# Load modules
#######################################################################################################################

message(STATUS "Assumed to be installed for normal development: git, wget, make")
message(STATUS "Assumed to be installed for tools around development: python")
message(STATUS "Assumed to be installed to generate factory images: pass, srec_cat, libarchive-zip-perl")

message(STATUS "The CPU on which CMake is running, arch=${CMAKE_HOST_SYSTEM_PROCESSOR}")

# The source dir is the parent directory, the binary directory is the build/ directory (out of build)
message(STATUS "Source dir ${CMAKE_SOURCE_DIR}")
message(STATUS "Build dir ${CMAKE_BINARY_DIR}")
message(STATUS "Installation dir with binaries ${CMAKE_INSTALL_PREFIX}")

set(BLUENET_CMAKE_MODULES_PATH "${CMAKE_SOURCE_DIR}/source/conf/cmake/modules/")
message(STATUS "Search for CMake modules in ${BLUENET_CMAKE_MODULES_PATH}")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${BLUENET_CMAKE_MODULES_PATH}")

# FindGit and FindWget are standard cmake modules, see
# https://cmake.org/cmake/help/latest/manual/cmake-modules.7.html
find_package(Git)
if(NOT Git_FOUND)
	message(FATAL_ERROR "Installation requires git, install with something like `sudo apt install git`")
endif()

find_package(Wget)
if(NOT WGET_FOUND)
	message(FATAL_ERROR "Installation requires wget, install with something like `sudo apt install wget`")
endif()

find_package(gcc_arm_none_eabi)
if(NOT gcc_arm_none_eabi_FOUND)
	message(FATAL_ERROR "Installation requires cross-compiler")
endif()

if(FACTORY_IMAGE)
	find_package(pass)
	if(NOT PASS_FOUND)
		message(STATUS "You should install pass, e.g. by sudo apt install pass.")
		message(FATAL_ERROR "The utility pass is required to sign release firmware.")
	else()
		message(STATUS "Found pass utility ${PASS_EXECUTABLE}")
	endif()
	find_package(srecord)
	if(NOT SRECORD_FOUND)
		message(STATUS "You should install srecord, e.g. by sudo apt install srecord.")
		message(FATAL_ERROR "The tool srecord (srec_cat) is required to build release firmware.")
	else()
		message(STATUS "Found srecord ${SRECORD_EXECUTABLE}")
	endif()
endif()

set(COMPILE_ON_ARM 0)
if("${CMAKE_HOST_SYSTEM_PROCESSOR}" MATCHES "arm*" OR "${CMAKE_HOST_SYSTEM_PROCESSOR}" MATCHES "aarch64")
	message(STATUS "Host has an ARM processor")
	set(COMPILE_ON_ARM 1)
endif()

if(REQUIRE_PYTHON)
	# This utility is able to find either python2 or python3. Set virtual env variant to be used first.
	set(Python_FIND_VIRTUALENV FIRST)
	find_package(Python COMPONENTS Interpreter REQUIRED)
	if(NOT Python_FOUND)
		message(FATAL_ERROR "No python interpreter found. First install python")
	else()
		message(STATUS "Use python interpreter: ${Python_EXECUTABLE}")
	endif()
	if(DEFINED ENV{VIRTUAL_ENV} OR DEFINED ENV{CONDA_PREFIX})
		message(STATUS "Virtual environment detected")
	else()
		message(STATUS "Virtual environment not detected. Not recommended.")
	endif()
endif()

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

load_configuration("${DEFAULT_CONFIGURATION_FILE}" CMAKE_BLUENET_ARGS)
load_configuration("${BOARD_TARGET_CONFIGURATION_FILE}" CMAKE_BLUENET_ARGS)
load_configuration("${BOARD_TARGET_OVERWRITE_CONFIGURATION_FILE}" CMAKE_BLUENET_ARGS)

# From now on you can use information from the config files, not above!

#######################################################################################################################
# Overwrite configuration values for just a few options
#######################################################################################################################

if(OVERWRITE_COMPILER_VERSION)
	message(STATUS "Overwrite compiler version with ${OVERWRITE_COMPILER_VERSION}")
	set(COMPILER_VERSION "${OVERWRITE_COMPILER_VERSION}")
else()
	message(STATUS "Compiler version to use: ${COMPILER_VERSION}")
endif()

#######################################################################################################################
# Create some convenience variables that use configuration values
#######################################################################################################################

# Create full semver string to be used to store patches and patched files
set(NORDIC_SDK_VERSION_FULL "${NORDIC_SDK_VERSION_MAJOR}.${NORDIC_SDK_VERSION_MINOR}.${NORDIC_SDK_VERSION_PATCH}")

# Create full semver string for mesh
set(MESH_SDK_VERSION_FULL "${MESH_SDK_VERSION_MAJOR}.${MESH_SDK_VERSION_MINOR}.${MESH_SDK_VERSION_PATCH}")

# Location of adjusted version of the SDK that can be used on a host system
set(HOST_NRF5_DIR "${WORKSPACE_DIR}/tools/nrf5_host_sdk/${NORDIC_SDK_VERSION_FULL}")

#######################################################################################################################
# Print some important configuration values that have been loaded above
#######################################################################################################################

message(STATUS "Board target: ${BOARD_TARGET}")
if(DEFINED NORDIC_HARDWARE_BOARD)
	message(STATUS "Nordic hardware development board: ${NORDIC_HARDWARE_BOARD}")
else()
	message(STATUS "Not on a Nordic hardware development board")
endif()

#######################################################################################################################
# Download urls and hashes
#######################################################################################################################

set(NRF5_SDK_REPOSITORY "https://github.com/crownstone/nRF5_SDK")

set(MESH_SDK_REPOSITORY "https://github.com/crownstone/nRF5-SDK-for-Mesh")

#######################################################################################################################
# Get proper git tags / hashes
#######################################################################################################################

# The Nordic SDK
if (NORDIC_SDK_VERSION EQUAL 15 AND NORDIC_SDK_VERSION_MINOR EQUAL 3 AND NORDIC_SDK_VERSION_PATCH EQUAL 0)
elseif (NORDIC_SDK_VERSION EQUAL 17 AND NORDIC_SDK_VERSION_MINOR EQUAL 1 AND NORDIC_SDK_VERSION_PATCH EQUAL 0)
else()
	message(FATAL_ERROR "Unknown SDK version: ${NORDIC_SDK_VERSION}")
endif()

if(NORDIC_SDK_GIT_HASH)
	message(STATUS "Will use git hash ${NORDIC_SDK_GIT_HASH} of repos ${NORDIC_SDK_REPOSITORY}")
	set(NORDIC_SDK_TARGET nordic_sdk_${NORDIC_SDK_GIT_HASH})
	set(NORDIC_SDK_GIT_TAG "${NORDIC_SDK_GIT_HASH}")
	set(NRF5_DIR "${WORKSPACE_DIR}/tools/nrf5_sdk/${NORDIC_SDK_GIT_HASH}")
else()
	message(STATUS "Use git tag ${NORDIC_SDK_VERSION_FULL} of repos ${NRF5_SDK_REPOSITORY}")
	set(NORDIC_SDK_TARGET nordic_sdk_${NORDIC_SDK_VERSION_FULL})
	set(NORDIC_SDK_GIT_TAG "origin/${NORDIC_SDK_VERSION_FULL}")
	set(NRF5_DIR "${WORKSPACE_DIR}/tools/nrf5_sdk/${NORDIC_SDK_VERSION_FULL}")
endif()

# Make some more variables available to the bluenet build
list(APPEND CMAKE_BLUENET_ARGS "-DNORDIC_SDK_VERSION_FULL:STRING=${NORDIC_SDK_VERSION_FULL}")

# The mesh
if(MESH_SDK_VERSION_MAJOR EQUAL 3 AND MESH_SDK_VERSION_MINOR EQUAL 2 AND MESH_SDK_VERSION_PATCH EQUAL 0)
elseif(MESH_SDK_VERSION_MAJOR EQUAL 5 AND MESH_SDK_VERSION_MINOR EQUAL 0 AND MESH_SDK_VERSION_PATCH EQUAL 0)
else()
	message(FATAL_ERROR "Unknown Mesh version: ${MESH_SDK_VERSION_FULL}")
endif()

if(MESH_SDK_GIT_HASH)
	message(STATUS "Will use git hash ${MESH_SDK_GIT_HASH} of repos ${MESH_SDK_REPOSITORY}")
	set(MESH_SDK_TARGET nordic_mesh_sdk_${MESH_SDK_GIT_HASH})
	set(MESH_SDK_GIT_TAG "${MESH_SDK_GIT_HASH}")
	set(MESH_SDK_DIR "${WORKSPACE_DIR}/tools/mesh_sdk/${MESH_SDK_GIT_HASH}")
else()
	message(STATUS "Will use git tag ${MESH_SDK_VERSION_FULL} of repos ${MESH_SDK_REPOSITORY}")
	set(MESH_SDK_TARGET nordic_mesh_sdk_${MESH_SDK_VERSION_FULL})
	set(MESH_SDK_GIT_TAG "origin/${MESH_SDK_VERSION_FULL}")
	set(MESH_SDK_DIR "${WORKSPACE_DIR}/tools/mesh_sdk/${MESH_SDK_VERSION_FULL}")
endif()

# Make some more variables available to the bluenet build
list(APPEND CMAKE_BLUENET_ARGS "-DMESH_SDK_VERSION_FULL:STRING=${MESH_SDK_VERSION_FULL}")

# Check for nrfjprog on a system from which we want to flash (not required for cloud-only tests)
if (REQUIRE_NRFJPROG)
	find_package(nrfjprog)
	if (NOT NRFJPROG_FOUND)
		message(FATAL_ERROR "Installation requires nrfjprog (see https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools)")
	endif()
endif()

# Check for JLinkExe on a system from which we want to flash
if (REQUIRE_JLINK)
	find_package(JLinkExe)
	if (NOT JLINKEXE_FOUND)
		message(FATAL_ERROR "Installation requires JLink (see https://www.segger.com/downloads/jlink/)")
	endif()
endif()

#######################################################################################################################
# Local names
#######################################################################################################################

# Softdevice is now set in CMakeBuild config file and a different one can be used depending on your target
# It should be something like:
#     SOFTDEVICE=s132_nrf52_7.0.1
#
set(SOFTDEVICE_HEX_FILE components/softdevice/s132/hex/${SOFTDEVICE}_softdevice.hex)

list(APPEND CMAKE_BLUENET_ARGS "-DSOFTDEVICE_HEX_FILE:STRING=${SOFTDEVICE_HEX_FILE}")

#######################################################################################################################
# Download dependencies
#######################################################################################################################

message(STATUS "Configure ${NORDIC_SDK_TARGET} target")
ExternalProject_Add(${NORDIC_SDK_TARGET}
	GIT_REPOSITORY ${NRF5_SDK_REPOSITORY}
	GIT_TAG ${NORDIC_SDK_GIT_TAG}
	GIT_CONFIG advice.detachedHead=false
	SOURCE_DIR ${NRF5_DIR}
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
	)

if(COMPILE_FOR_HOST)
	set(NORDIC_HOST_SDK_TARGET nordic_host_sdk${NORDIC_SDK_VERSION_FULL})
	message(STATUS "Configure ${NORDIC_HOST_SDK_TARGET} target")
	ExternalProject_Add(${NORDIC_HOST_SDK_TARGET}
		GIT_REPOSITORY https://github.com/mrquincle/nrf5_sdk
		GIT_TAG host
		GIT_CONFIG advice.detachedHead=false
		SOURCE_DIR ${HOST_NRF5_DIR}
		CONFIGURE_COMMAND ""
		BUILD_COMMAND ""
		INSTALL_COMMAND ""
		PATCH_COMMAND ""
		)
endif()

list(APPEND CMAKE_BLUENET_ARGS "-DNRF5_DIR:STRING=${NRF5_DIR}")

message(STATUS "Configure ${MESH_SDK_TARGET} target")
ExternalProject_Add(${MESH_SDK_TARGET}
	GIT_REPOSITORY ${MESH_SDK_REPOSITORY}
	GIT_TAG ${MESH_SDK_GIT_TAG}
	GIT_CONFIG advice.detachedHead=false
	SOURCE_DIR ${MESH_SDK_DIR}
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
	INSTALL_COMMAND ""
	PATCH_COMMAND ""
	)

list(APPEND CMAKE_BLUENET_ARGS "-DMESH_SDK_DIR:STRING=${MESH_SDK_DIR}")

set(COMPILER_PATH ${WORKSPACE_DIR}/tools/gcc_arm_none_eabi)
list(APPEND CMAKE_BLUENET_ARGS "-DCOMPILER_PATH:STRING=${COMPILER_PATH}")

if(FACTORY_IMAGE)
	message(WARNING "Make a factory image. The usual CMakeBuild.config files will not be used. "
		"They will be overwritten by the CMakeFactoryImage.config files. "
		"Make sure that you set -DFACTORY_IMAGE=0 when you continue normal development. ")

	if("${FACTORY_IMAGE_HARDWARE_CONFIG}" STREQUAL "")
		message(FATAL_ERROR "It seems you forgot to set -DFACTORY_IMAGE_HARDWARE_CONFIG")
	endif()
	if("${FACTORY_IMAGE_SOFTWARE_CONFIG}" STREQUAL "")
		message(FATAL_ERROR "It seems you forgot to set -DFACTORY_IMAGE_SOFTWARE_CONFIG")
	endif()
	if(NOT EXISTS "${RELEASE_REPOSITORY}")
		message(FATAL_ERROR "The release repository ${RELEASE_REPOSITORY} does not exist")
	endif()
	if(NOT EXISTS "${RELEASE_CANDIDATE_REPOSITORY}")
		message(FATAL_ERROR "The release candidate repository ${RELEASE_CANDIDATE_REPOSITORY} does not exist")
	endif()

	set(FACTORY_IMAGE_HARDWARE_CONFIGURATION_FILE ${WORKSPACE_DIR}/factory-images/hardware/${FACTORY_IMAGE_HARDWARE_CONFIG}/CMakeFactoryImage.config)
	set(FACTORY_IMAGE_SOFTWARE_CONFIGURATION_FILE ${WORKSPACE_DIR}/factory-images/software/${FACTORY_IMAGE_SOFTWARE_CONFIG}/CMakeFactoryImage.config)

	message(STATUS "Load hardware configuration file for factory image at ${FACTORY_IMAGE_HARDWARE_CONFIGURATION_FILE}")
	load_configuration("${FACTORY_IMAGE_HARDWARE_CONFIGURATION_FILE}" CMAKE_BLUENET_ARGS)

	message(STATUS "Load software configuration file for factory image at ${FACTORY_IMAGE_SOFTWARE_CONFIGURATION_FILE}")
	load_configuration("${FACTORY_IMAGE_SOFTWARE_CONFIGURATION_FILE}" CMAKE_BLUENET_ARGS)

	IF(EXISTS ${FACTORY_IMAGE_HARDWARE_CONFIGURATION_FILE})
		set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${FACTORY_IMAGE_HARDWARE_CONFIGURATION_FILE})
	ENDIF()
	IF(EXISTS ${FACTORY_IMAGE_SOFTWARE_CONFIGURATION_FILE})
		set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${FACTORY_IMAGE_SOFTWARE_CONFIGURATION_FILE})
	ENDIF()
endif()

# Setting INSTALL_DIR is not sufficient, also CMAKE_INSTALL_PREFIX is required
list(APPEND CMAKE_BLUENET_ARGS "-DCMAKE_INSTALL_PREFIX:FILEPATH=${WORKSPACE_DIR}/bin/${BOARD_TARGET}")

# Add cross-compilation flags
list(APPEND CMAKE_BLUENET_ARGS "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${WORKSPACE_DIR}/source/arm.toolchain.cmake")

if(VERBOSITY GREATER 5)
	message(STATUS "list: ${CMAKE_BLUENET_ARGS}")
endif()

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/${BOARD_TARGET}/stamp)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/${BOARD_TARGET}/tmp)

#######################################################################################################################
# Build encryption library (required by bootloader)
#######################################################################################################################

# We have added some commands to remove windows line endings and to set the exec bit
# Reported at https://devzone.nordicsemi.com/f/nordic-q-a/47429/bug-windows-line-ending-characters
add_custom_target(micro_eec
	COMMAND sed -i 's/\r//g' build_all.sh WORKING_DIRECTORY ${NRF5_DIR}/external/micro-ecc
	COMMAND chmod u+x build_all.sh WORKING_DIRECTORY ${NRF5_DIR}/external/micro-ecc
	COMMAND ${CMAKE_COMMAND} -E env "GNU_INSTALL_ROOT=${COMPILER_PATH}/bin/" "VERBOSE=0 MAKEFLAGS=-j1" ./build_all.sh WORKING_DIRECTORY ${NRF5_DIR}/external/micro-ecc
	COMMENT "Build micro-eec"
	DEPENDS
	${NORDIC_SDK_TARGET}
	)

add_custom_target(target_depend
	COMMAND cd ${BOARD_TARGET} && test -f "Makefile" && make depend || echo "-- First install"
	COMMENT "Run make depend in target directory"
	DEPENDS
	${NORDIC_SDK_TARGET}
	${MESH_SDK_TARGET}
	)

#######################################################################################################################
# Build bluenet as external project
#######################################################################################################################

if(NOT FACTORY_IMAGE)
	# Note that some more dependencies might be added later on by add_dependencies()
	ExternalProject_Add(bluenet
		SOURCE_DIR ${WORKSPACE_DIR}/source
		BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${BOARD_TARGET}
		INSTALL_DIR ${WORKSPACE_DIR}/bin/${BOARD_TARGET}
		STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/${BOARD_TARGET}/stamp
		TMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/${BOARD_TARGET}/tmp
		CMAKE_CACHE_ARGS "${CMAKE_BLUENET_ARGS}"
		CMAKE_ARGS "${CMAKE_BLUENET_ARGS}"
		BUILD_ALWAYS 1
		DEPENDS
			micro_eec
			target_depend
		)
endif()

set(BOARD_CONFIG_DIR_FULL "${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${BOARD_CONFIG_DIR_FULL}/CMakeBuild.config)
if(EXISTS ${BOARD_CONFIG_DIR_FULL}/CMakeBuild.overwrite.config)
	set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${BOARD_CONFIG_DIR_FULL}/CMakeBuild.overwrite.config)
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

set(WRITE_SOFTDEVICE_TARGET write_softdevice)
message(STATUS "Configure ${WRITE_SOFTDEVICE_TARGET} target")
add_custom_target(${WRITE_SOFTDEVICE_TARGET}
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=WRITE_BINARY" "-DBINARY=${NRF5_DIR}/${SOFTDEVICE_HEX_FILE}" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Flashes softdevice to remote target board"
	DEPENDS ${NRFJPROG_TARGET}
	)

set(WRITE_MBR_PARAM_ADDRESS_TARGET write_mbr_param_address)
message(STATUS "Configure ${WRITE_MBR_PARAM_ADDRESS_TARGET} target")
add_custom_target(${WRITE_MBR_PARAM_ADDRESS_TARGET}
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=WRITE" "-DADDRESS=0x10001018" "-DVALUE=0x0007E000" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Writes MBR param page address to UICR using nrfjprog"
	DEPENDS ${NRFJPROG_TARGET}
	)

set(READ_SOFTDEVICE_VERSION_TARGET read_softdevice_version)
message(STATUS "Configure ${READ_SOFTDEVICE_VERSION_TARGET} target")
add_custom_target(${READ_SOFTDEVICE_VERSION_TARGET}
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=READ" "-DADDRESS=0x000300C" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Reads version of softdevice from the hardware itself through nrfjprog"
	DEPENDS ${NRFJPROG_TARGET}
	)

set(ERASE_TARGET erase)
message(STATUS "Configure ${ERASE_TARGET} target")
add_custom_target(${ERASE_TARGET}
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=ERASE" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Erase everything on target board"
	DEPENDS ${NRFJPROG_TARGET}
	)

set(RESET_TARGET reset)
message(STATUS "Configure ${RESET_TARGET} target")
add_custom_target(${RESET_TARGET}
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=RESET" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Reset target board"
	DEPENDS ${NRFJPROG_TARGET}
	)

#######################################################################################################################
# Forward commands for a particular board towards its build directory
#######################################################################################################################

# Will not be called automatically (this is done through the external project bluenet)
message(STATUS "Configure ${BOARD_TARGET} target")
add_custom_target(${BOARD_TARGET}
	COMMAND make
	WORKING_DIRECTORY ${BOARD_TARGET}
	DEPENDS
	bluenet
	COMMENT "Build application"
	)

set(DEBUG_APPLICATION_TARGET debug_application)
message(STATUS "Configure ${DEBUG_APPLICATION_TARGET} target")
add_custom_target(${DEBUG_APPLICATION_TARGET}
	COMMAND make debug_application
	WORKING_DIRECTORY ${BOARD_TARGET}
	DEPENDS
	bluenet
	COMMENT "Debug application ${BOARD_TARGET}"
	)

set(WRITE_BOARD_VERSION_TARGET write_board_version)
message(STATUS "Configure ${WRITE_BOARD_VERSION_TARGET} target")
add_custom_target(${WRITE_BOARD_VERSION_TARGET}
	COMMAND make write_board_version
	WORKING_DIRECTORY ${BOARD_TARGET}
	DEPENDS
	${NRFJPROG_TARGET}
	bluenet
	COMMENT "Write board version for ${BOARD_TARGET}"
	)

set(WRITE_APPLICATION_TARGET write_application)
message(STATUS "Configure ${WRITE_APPLICATION_TARGET} target")
add_custom_target(${WRITE_APPLICATION_TARGET}
	COMMAND make write_application
	WORKING_DIRECTORY ${BOARD_TARGET}
	DEPENDS
	${NRFJPROG_TARGET}
	bluenet
	COMMENT "Write application to ${BOARD_TARGET}"
	)

set(WRITE_BOOTLOADER_TARGET write_bootloader)
message(STATUS "Configure ${WRITE_BOOTLOADER_TARGET} target")
add_custom_target(${WRITE_BOOTLOADER_TARGET}
	COMMAND make write_bootloader
	WORKING_DIRECTORY ${BOARD_TARGET}
	DEPENDS
	bluenet
	COMMENT "Write bootloader to ${BOARD_TARGET}"
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
set(CREATE_GIT_RELEASE_TARGET create_git_release)
message(STATUS "Configure ${CREATE_GIT_RELEASE_TARGET} target")
add_custom_target(${CREATE_GIT_RELEASE_TARGET}
	COMMAND ${CMAKE_COMMAND} -DDEFAULT_MODULES_PATH=${DEFAULT_MODULES_PATH} -DWORKSPACE_DIR=${WORKSPACE_DIR} -P ${DEFAULT_MODULES_PATH}/release.cmake
	COMMENT "Create git release"
	)

set(CREATE_GIT_RELEASE_BOOTLOADER_TARGET create_git_release_bootloader)
message(STATUS "Configure ${CREATE_GIT_RELEASE_BOOTLOADER_TARGET} target")
add_custom_target(${CREATE_GIT_RELEASE_BOOTLOADER_TARGET}
	COMMAND ${CMAKE_COMMAND} -DDEFAULT_MODULES_PATH=${DEFAULT_MODULES_PATH} -DWORKSPACE_DIR=${WORKSPACE_DIR} -P ${DEFAULT_MODULES_PATH}/release_bootloader.cmake
	COMMENT "Create git release for bootloader"
	)


#######################################################################################################################
# Some more utility functions
#######################################################################################################################

add_custom_target(list_jlinks
	COMMAND JLinkExe -commanderscript list.jlink
	WORKING_DIRECTORY ${WORKSPACE_DIR}/source/conf/jlink
	DEPENDS ${JLINK_TARGET}
	COMMENT "List JLink devices"
	)

add_custom_target(nrfconnect_core_setup
	COMMAND ${CMAKE_COMMAND} -E echo "Run in separate shell. This is an electron app, debug with Ctrl+Shift+I"
	COMMAND npm run dev
	WORKING_DIRECTORY ${WORKSPACE_DIR}/tools/nrfconnect/pc-nrfconnect-core
	COMMENT "Run nrf connect core development"
	)

add_custom_target(nrfconnect_core
	COMMAND ${CMAKE_COMMAND} -E echo "Run in separate shell. This is an electron app, debug with Ctrl+Shift+I"
	COMMAND npm run app
	WORKING_DIRECTORY ${WORKSPACE_DIR}/tools/nrfconnect/pc-nrfconnect-core
	COMMENT "Run nrf connect core"
	)

add_custom_target(nrfconnect_programmer
	COMMAND npm run app
	WORKING_DIRECTORY ${WORKSPACE_DIR}/tools/nrfconnect_apps/pc-nrfconnect-programmer
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
		message(STATUS "Product family type: ${PRODUCT_FAMILY_TYPE}")
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

	set(CONSTANT_STRING "-l-e-constant")
	if(SRECORD_NEW_VERSION)
		set(CONSTANT_STRING "-constant-l-e")
	endif()

	math(EXPR HARDWARE_BOARD_ADDRESS_END "${HARDWARE_BOARD_ADDRESS} + 4" OUTPUT_FORMAT HEXADECIMAL)
	math(EXPR FAMILY_MARKET_TYPE_ADDRESS_END "${FAMILY_MARKET_TYPE_ADDRESS} + 4" OUTPUT_FORMAT HEXADECIMAL)
	math(EXPR MAJOR_MINOR_PATCH_ADDRESS_END "${MAJOR_MINOR_PATCH_ADDRESS} + 4" OUTPUT_FORMAT HEXADECIMAL)
	math(EXPR PROD_DATE_HOUSING_ADDRESS_END "${PROD_DATE_HOUSING_ADDRESS} + 4" OUTPUT_FORMAT HEXADECIMAL)

	set(SREC_CAT_UICR_BOOTLOADER        "-exclude;0x10001014;0x10001018;-generate;0x10001014;0x10001018;${CONSTANT_STRING};${UICR_BOOTLOADER_ADDRESS};4")
	set(SREC_CAT_MBR_SETTINGS           "-exclude;0x10001018;0x1000101C;-generate;0x10001018;0x1000101C;${CONSTANT_STRING};${MBR_SETTINGS};4")
	set(SREC_CAT_HARDWARE_BOARD         "-exclude;${HARDWARE_BOARD_ADDRESS};${HARDWARE_BOARD_ADDRESS_END};-generate;${HARDWARE_BOARD_ADDRESS};${HARDWARE_BOARD_ADDRESS_END};${CONSTANT_STRING};${HARDWARE_BOARD_HEX};4")
	set(SREC_CAT_PRODUCT_FAMILY_TYPE    "-exclude;${FAMILY_MARKET_TYPE_ADDRESS};${FAMILY_MARKET_TYPE_ADDRESS_END};-generate;${FAMILY_MARKET_TYPE_ADDRESS};${FAMILY_MARKET_TYPE_ADDRESS_END};${CONSTANT_STRING};${PRODUCT_FAMILY_TYPE};4")
	set(SREC_CAT_MAJOR_MINOR_PATCH      "-exclude;${MAJOR_MINOR_PATCH_ADDRESS};${MAJOR_MINOR_PATCH_ADDRESS_END};-generate;${MAJOR_MINOR_PATCH_ADDRESS};${MAJOR_MINOR_PATCH_ADDRESS_END};${CONSTANT_STRING};${MAJOR_MINOR_PATCH};4")
	set(SREC_CAT_PRODUCTION_RUN_HOUSING "-exclude;${PROD_DATE_HOUSING_ADDRESS};${PROD_DATE_HOUSING_ADDRESS_END};-generate;${PROD_DATE_HOUSING_ADDRESS};${PROD_DATE_HOUSING_ADDRESS_END};${CONSTANT_STRING};${PRODUCTION_RUN_HOUSING};4")

	string(TIMESTAMP FACTORY_IMAGE_TIMESTAMP "%Y-%U")

	fill(${FACTORY_IMAGE_SOFTWARE_VERSION} FACTORY_IMAGE_SOFTWARE_VERSION 4 "0" "")

	# Check some stuff on the previously generated binaries that go into the factory image
	set(CHECK_FACTORY_IMAGE_CONDITIONS check_factory_image_conditions)
	message(STATUS "Check factory image conditions before generating it")
	message(STATUS "This requires crc32 on the command line (e.g. apt install libarchive-zip-perl")
	string(REGEX REPLACE "[.]hex$" ".bin" FACTORY_IMAGE_APPLICATION_BIN_FILE "${FACTORY_IMAGE_APPLICATION_HEX_FILE}")
	message(STATUS "We will compare the crc32 of ${FACTORY_IMAGE_APPLICATION_BIN_FILE} with the checksum in ${FACTORY_IMAGE_BOOTLOADER_SETTINGS_HEX_FILE}")
	add_custom_target(${CHECK_FACTORY_IMAGE_CONDITIONS}
		COMMAND test -f "${FACTORY_IMAGE_APPLICATION_BIN_FILE}" && exit 0 || echo "Application missing. No such file: ${FACTORY_IMAGE_APPLICATION_BIN_FILE}" && echo && exit 1
		COMMAND test -f "${FACTORY_IMAGE_BOOTLOADER_SETTINGS_HEX_FILE}" && exit 0 || echo "Bootloader settings missing. No such file: ${FACTORY_IMAGE_BOOTLOADER_SETTINGS_HEX_FILE}" && echo && exit 1
		COMMAND nrfutil settings display "${FACTORY_IMAGE_BOOTLOADER_SETTINGS_HEX_FILE}" | grep 'Application CRC' | cut -f2 -d':' | tr -d ' ' | tr '[:upper:]' '[:lower:]' | sed 's/0x//g' > "${CMAKE_BINARY_DIR}/.nrfutil_app_crc"
		COMMAND crc32 "${FACTORY_IMAGE_APPLICATION_BIN_FILE}" > "${CMAKE_BINARY_DIR}/.cli_app_crc"
		COMMAND diff ${CMAKE_BINARY_DIR}/.nrfutil_app_crc ${CMAKE_BINARY_DIR}/.cli_app_crc 1>/dev/null 2>&1 && echo '-- Application checksum matches' && exit 0 || echo "Application CRC does not match!" && exit 1
	)

	set(FACTORY_IMAGE_OUTPUT_FILE_NAME "${FACTORY_IMAGE_HARDWARE_CONFIG}/software_${FACTORY_IMAGE_SOFTWARE_VERSION}/factory-image.hex")
	set(FACTORY_IMAGE_PATH_NAME factory-images/${FACTORY_IMAGE_HARDWARE_CONFIG}/software_${FACTORY_IMAGE_SOFTWARE_VERSION})

	# Add this target by default to make all when in factory image creation mode
	set(GENERATE_FACTORY_IMAGE_TARGET generate_factory_image)
	message(STATUS "Configure ${GENERATE_FACTORY_IMAGE_TARGET} target")
	add_custom_target(${GENERATE_FACTORY_IMAGE_TARGET} ${ADD_TO_DEFAULT_BUILD_TARGET}
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
		DEPENDS ${CHECK_FACTORY_IMAGE_CONDITIONS}
		)

	message(STATUS "Install binary into ${CMAKE_BINARY_DIR}/${FACTORY_IMAGE_PATH_NAME} dir")
	install(DIRECTORY "${CMAKE_BINARY_DIR}/${FACTORY_IMAGE_PATH_NAME}/" DESTINATION "${FACTORY_IMAGE_PATH_NAME}" FILES_MATCHING PATTERN "*")
endif()

#######################################################################################################################
# Build/compile for host
#######################################################################################################################

list(APPEND CMAKE_BLUENET_HOST_ARGS "-DHOST_TARGET:OPTION=ON")
list(APPEND CMAKE_BLUENET_HOST_ARGS "-DDISABLE_NRF_HEADERS:OPTION=OFF")
list(APPEND CMAKE_BLUENET_HOST_ARGS "-DNRF5_DIR:STRING=${HOST_NRF5_DIR}")
list(APPEND CMAKE_BLUENET_HOST_ARGS "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}")
list(APPEND CMAKE_BLUENET_HOST_ARGS "-DNORDIC_SDK_VERSION_FULL:STRING=${NORDIC_SDK_VERSION_FULL}")

list(REMOVE_ITEM CMAKE_BLUENET_ARGS "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=${WORKSPACE_DIR}/source/arm.toolchain.cmake")

if(COMPILE_FOR_HOST)
	set(BLUENET_HOST_TARGET bluenet_host)
	message(STATUS "Configure ${BLUENET_HOST_TARGET} target")
	ExternalProject_Add(${BLUENET_HOST_TARGET}
		SOURCE_DIR ${WORKSPACE_DIR}/host
		BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/host/${BOARD_TARGET}
		INSTALL_DIR ${WORKSPACE_DIR}/bin/host/${BOARD_TARGET}
		STAMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/host/${BOARD_TARGET}/stamp
		TMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/host/${BOARD_TARGET}/tmp
		CMAKE_ARGS "${CMAKE_BLUENET_HOST_ARGS}"
		BUILD_ALWAYS 1
		DEPENDS ${NORDIC_HOST_SDK_TARGET}
		)
endif()

