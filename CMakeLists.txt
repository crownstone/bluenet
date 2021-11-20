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
option(COMPILE_FOR_HOST                          "Compile for host"                      OFF)
option(PYTHON_SETUP_VENV                         "Setup python virtual env for user"     OFF)
option(AUTO_UPDATE_TOOLS                         "Auto-update tools on building bluenet" ON)

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

set(OVERWRITE_COMPILER_VERSION                   "" CACHE STRING "Overwrite compiler version")

# Set build type to debug, unless specified otherwise. This prevents accidental release builds.
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE                         Debug)
endif()

# Overwrite the typical install prefix
set(CMAKE_INSTALL_PREFIX                         ${CMAKE_SOURCE_DIR}/bin)

set(PYTHON_REQUIRED OFF)
if(DOWNLOAD_CSUTIL OR DOWNLOAD_NRFUTIL OR DOWNLOAD_BLUENET_LIB_LOGS)
	message(STATUS "One of the download options requires python")
	set(PYTHON_REQUIRED ON)
else()
	message(STATUS "No download option which requires python is enabled")
endif()

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

message(STATUS "CPU CMake is running on: ${CMAKE_HOST_SYSTEM_PROCESSOR}")

# The source dir is the parent directory, the binary directory is the build/ directory (out of build)
message(STATUS "source dir: ${CMAKE_SOURCE_DIR}")
message(STATUS "bin dir: ${CMAKE_BINARY_DIR}")
message(STATUS "install dir: ${CMAKE_INSTALL_PREFIX}")

find_package(Git REQUIRED)

set(COMPILE_ON_ARM 0)
if("${CMAKE_HOST_SYSTEM_PROCESSOR}" MATCHES "arm*" OR "${CMAKE_HOST_SYSTEM_PROCESSOR}" MATCHES "aarch64")
	message(STATUS "Host has an ARM processor")
	set(COMPILE_ON_ARM 1)
endif()

if(PYTHON_REQUIRED)
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
		if(PYTHON_SETUP_VENV)
			message(STATUS "No virtual environment detected, will configure it.")
			message(STATUS "Then call `make python_setup_venv` to set it up")
			message(STATUS "Also activate the environment")
		else()
			message(WARNING "Virtual environment not detected! Set it up or use -DPYTHON_SETUP_VENV=ON.")
		endif()
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

# Location of the SDK to be used
set(NRF5_DIR "${WORKSPACE_DIR}/tools/nrf5_sdk/${NORDIC_SDK_VERSION_FULL}")

# Location of adjusted version of the SDK that can be used on a host system
set(HOST_NRF5_DIR "${WORKSPACE_DIR}/tools/nrf5_host_sdk/${NORDIC_SDK_VERSION_FULL}")

# Location of mesh SDK
set(MESH_SDK_DIR "${WORKSPACE_DIR}/tools/mesh_sdk/${MESH_SDK_VERSION_FULL}")

# Make some more variables available to the bluenet build
list(APPEND CMAKE_BLUENET_ARGS "-DNORDIC_SDK_VERSION_FULL:STRING=${NORDIC_SDK_VERSION_FULL}")
list(APPEND CMAKE_BLUENET_ARGS "-DMESH_SDK_VERSION_FULL:STRING=${MESH_SDK_VERSION_FULL}")

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

# If you update the version of a .zip file, also update the md5 hash.
# Get the hashes through `md5sum downloads/*`

set(NRF5_SDK_REPOSITORY "https://github.com/crownstone/nRF5_SDK")

set(NORDIC_DOWNLOAD_URL https://www.nordicsemi.com/-/media/Software-and-other-downloads)

# The Nordic SDK
if (NORDIC_SDK_VERSION EQUAL 15 AND NORDIC_SDK_VERSION_MINOR EQUAL 3 AND NORDIC_SDK_VERSION_PATCH EQUAL 0)
	message(STATUS "Use git tag ${NORDIC_SDK_VERSION_FULL} of repos ${NRF5_SDK_REPOSITORY}")
elseif (NORDIC_SDK_VERSION EQUAL 17 AND NORDIC_SDK_VERSION_MINOR EQUAL 1 AND NORDIC_SDK_VERSION_PATCH EQUAL 0)
	message(STATUS "Use git tag ${NORDIC_SDK_VERSION_FULL} of repos ${NRF5_SDK_REPOSITORY}")
else()
	message(FATAL_ERROR "Unknown SDK version: ${NORDIC_SDK_VERSION}")
endif()

set(NORDIC_NRF5_SDK_DOWNLOAD_URL ${NORDIC_DOWNLOAD_URL}/${NORDIC_SDK_URL_FILE})

set(MESH_SDK_REPOSITORY "https://github.com/crownstone/nRF5-SDK-for-Mesh")

# The mesh
if (MESH_SDK_VERSION_MAJOR EQUAL 3 AND MESH_SDK_VERSION_MINOR EQUAL 2 AND MESH_SDK_VERSION_PATCH EQUAL 0)
	message(STATUS "Will use git tag ${MESH_SDK_VERSION_FULL} of repos ${MESH_SDK_REPOSITORY}")
else()
	message(FATAL_ERROR "Unknown Mesh version: ${MESH_SDK_VERSION_FULL}")
endif()

# The nrfjprog utility
if(COMPILE_ON_ARM)
	set(NRFJPROG_DOWNLOAD_URL ${NORDIC_DOWNLOAD_URL}/Desktop-software/nRF-command-line-tools/sw/Versions-10-x-x/10-15-0/nrf-command-line-tools-10.15.0_arm.zip)
	set(NRFJPROG_DEB_FILE nrf-command-line-tools_10.15.0_armhf.deb)
	set(NRFJPROG_MD5 49d0a47dc197f943502b164460bdd3e1)
else()
	set(NRFJPROG_DOWNLOAD_URL ${NORDIC_DOWNLOAD_URL}/Desktop-software/nRF-command-line-tools/sw/Versions-10-x-x/10-14-0/nRF-Command-Line-Tools_10_14_0_Linux64.zip)
	set(NRFJPROG_DEB_FILE nrf-command-line-tools_10.14.0_amd64.deb)
	set(NRFJPROG_MD5 8a049bacc67519561b77e014b652d5df)
endif()

# The cross-compiler
if(COMPILE_ON_ARM)
	if("${COMPILER_VERSION}" STREQUAL "10.3-2021.10")
		set(GCC_ARM_NONE_EABI_DOWNLOAD_URL https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-aarch64-linux.tar.bz2)
		set(GCC_ARM_NONE_EABI_MD5 3fe3d8bb693bd0a6e4615b6569443d0d)
	else()
		message(FATAL_ERROR "Unknown compiler version: ${COMPILER_VERSION} for ${CMAKE_HOST_SYSTEM_PROCESSOR}")
	endif()
else()
	if("${COMPILER_VERSION}" STREQUAL "8-2019-q3-update")
		set(GCC_ARM_NONE_EABI_DOWNLOAD_URL https://developer.arm.com/-/media/Files/downloads/gnu-rm/8-2019q3/RC1.1/gcc-arm-none-eabi-8-2019-q3-update-linux.tar.bz2?revision=c34d758a-be0c-476e-a2de-af8c6e16a8a2?product=GNU%20Arm%20Embedded%20Toolchain,64-bit,,Linux,8-2019-q3-update)
		set(GCC_ARM_NONE_EABI_MD5 6341f11972dac8de185646d0fbd73bfc)
	elseif("${COMPILER_VERSION}" STREQUAL "10.3-2021.10")
		set(GCC_ARM_NONE_EABI_DOWNLOAD_URL https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2)
		set(GCC_ARM_NONE_EABI_MD5 2383e4eb4ea23f248d33adc70dc3227e)
	else()
		message(FATAL_ERROR "Unknown compiler version: ${COMPILER_VERSION}")
	endif()
endif()

# The JLink tool
set(JLINK_DOWNLOAD_URL https://www.segger.com/downloads/jlink)
if(COMPILE_ON_ARM)
	message(STATUS "Get ARM (64-bit) version of JLink tool")
	set(JLINK_DEB_FILE JLink_Linux_V758_arm64.deb)
	set(JLINK_MD5 f6a74b6a134fed8036238b107504369b)
else()
	set(JLINK_DEB_FILE JLink_Linux_V754b_x86_64.deb)
	set(JLINK_MD5 999c69c040168cffb272850fc62a3708)
endif()
set(JLINK_LINUX_DOWNLOAD_URL ${JLINK_DOWNLOAD_URL}/${JLINK_DEB_FILE})

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

if(PYTHON_SETUP_VENV)
	set(PYTHON_CROWNSTONE_VENV_PATH ${WORKSPACE_DIR}/tools/python/crownstone_env)

	# Add python virtual env setup on Ubuntu systems
	# We first use `apt-cache` so that we only run `apt install` when there is actually a candidate
	# that is different from what is installed.
	set(PYTHON_SETUP_TARGET python_setup_venv)
	add_custom_target(python_setup_venv
		COMMAND ${CMAKE_COMMAND} -E echo "-- Install python3-venv"
		COMMAND apt-cache policy python3-venv > ${CMAKE_BINARY_DIR}/.apt-cache
		COMMAND cat ${CMAKE_BINARY_DIR}/.apt-cache | grep 'Installed:' | cut -f2 -d':' > ${CMAKE_BINARY_DIR}/.apt-cache.installed
		COMMAND cat ${CMAKE_BINARY_DIR}/.apt-cache | grep 'Candidate:' | cut -f2 -d':' > ${CMAKE_BINARY_DIR}/.apt-cache.candidate
		COMMAND diff ${CMAKE_BINARY_DIR}/.apt-cache.candidate ${CMAKE_BINARY_DIR}/.apt-cache.installed 1>/dev/null 2>&1 && echo '-- Already uptodate' || ${SUPERUSER_SWITCH} apt-get install -y python3-venv
		COMMAND ${CMAKE_COMMAND} -E make_directory ${PYTHON_CROWNSTONE_VENV_PATH}
		COMMAND ${Python_EXECUTABLE} -m venv ${PYTHON_CROWNSTONE_VENV_PATH}
		COMMAND ${CMAKE_COMMAND} -E echo "-- Make sure to run the following command:"
		COMMAND ${CMAKE_COMMAND} -E echo "--  source ${PYTHON_CROWNSTONE_VENV_PATH}/bin/activate"
		COMMAND ${CMAKE_COMMAND} -E echo "-- You can deactivate on the command line:"
		COMMAND ${CMAKE_COMMAND} -E echo "--  deactivate"
		WORKING_DIRECTORY ${WORKSPACE_DIR}
		COMMENT "Run python venv setup"
		)

	# Use a cmake -P structure to check actual ENV variables
	set(PYTHON_TEST_VENV_TARGET python_test_venv)
	add_custom_target(python_test_venv
		COMMAND ${CMAKE_COMMAND} -E echo "-- Test for venv"
		COMMAND ${CMAKE_COMMAND} -DPYTHON_CROWNSTONE_VENV_PATH=${PYTHON_CROWNSTONE_VENV_PATH} -P ${DEFAULT_MODULES_PATH}/test_python_venv.cmake
		WORKING_DIRECTORY ${WORKSPACE_DIR}
		COMMENT "Run python venv test"
		)
endif()

set(NORDIC_SDK_TARGET nordic_sdk${NORDIC_SDK_VERSION_FULL})
ExternalProject_Add(${NORDIC_SDK_TARGET}
	GIT_REPOSITORY ${NRF5_SDK_REPOSITORY}
	GIT_SHALLOW 1
	GIT_TAG origin/${NORDIC_SDK_VERSION_FULL}
	GIT_CONFIG advice.detachedHead=false
	SOURCE_DIR ${NRF5_DIR}
	CONFIGURE_COMMAND ${CMAKE_COMMAND} -E echo "Skipping configure step."
	BUILD_COMMAND ${CMAKE_COMMAND} -E echo "Skipping build step."
	INSTALL_COMMAND ${CMAKE_COMMAND} -E echo "Skipping install step."
	)

if(COMPILE_FOR_HOST)
	set(NORDIC_HOST_SDK_TARGET nordic_host_sdk${NORDIC_SDK_VERSION_FULL})
	ExternalProject_Add(${NORDIC_HOST_SDK_TARGET}
		GIT_REPOSITORY https://github.com/mrquincle/nrf5_sdk
		GIT_SHALLOW 1
		GIT_TAG host
		GIT_CONFIG advice.detachedHead=false
		SOURCE_DIR ${HOST_NRF5_DIR}
		CONFIGURE_COMMAND ${CMAKE_COMMAND} -E echo "Skipping configure step."
		BUILD_COMMAND ${CMAKE_COMMAND} -E echo "Skipping build step."
		INSTALL_COMMAND ${CMAKE_COMMAND} -E echo "Skipping install step."
		)
endif()

list(APPEND CMAKE_BLUENET_ARGS "-DNRF5_DIR:STRING=${NRF5_DIR}")

set(MESH_SDK_TARGET nordic_mesh_sdk${MESH_SDK_VERSION_FULL})
ExternalProject_Add(${MESH_SDK_TARGET}
	GIT_REPOSITORY ${MESH_SDK_REPOSITORY}
	GIT_SHALLOW 1
	GIT_TAG origin/${MESH_SDK_VERSION_FULL}
	GIT_CONFIG advice.detachedHead=false
	SOURCE_DIR ${MESH_SDK_DIR}
	CONFIGURE_COMMAND ${CMAKE_COMMAND} -E echo "Skipping configure step."
	BUILD_COMMAND ${CMAKE_COMMAND} -E echo "Skipping build step."
	INSTALL_COMMAND ${CMAKE_COMMAND} -E echo "Skipping install step."
	)

list(APPEND CMAKE_BLUENET_ARGS "-DMESH_SDK_DIR:STRING=${MESH_SDK_DIR}")

if(DOWNLOAD_JLINK)
	set(JLINK_TARGET jlink)
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
list(APPEND CMAKE_BLUENET_ARGS "-DCOMPILER_PATH:STRING=${COMPILER_PATH}")

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
	set(NRFUTIL_TARGET nrfutil)
	add_custom_target(nrfutil
		COMMAND ${CMAKE_COMMAND} -E echo "-- Add nrfutil through pip"
		COMMAND ${Python_EXECUTABLE} -m pip -q install wheel nrfutil
		WORKING_DIRECTORY ${WORKSPACE_DIR}
		COMMENT "Install nrfutil"
		DEPENDS ${PYTHON_SETUP_TARGET} ${PYTHON_TEST_VENV_TARGET}
		)
endif()

if(DOWNLOAD_NRFCONNECT)
	set(NRFCONNECT_CORE_TARGET git_nrfconnect_core)
	ExternalProject_Add(git_nrfconnect_core
		GIT_REPOSITORY https://github.com/NordicSemiconductor/pc-nrfconnect-core
		GIT_CONFIG advice.detachedHead=false
		SOURCE_DIR ${WORKSPACE_DIR}/tools/nrfconnect/pc-nrfconnect-core
		BINARY_DIR ${WORKSPACE_DIR}/tools/nrfconnect/pc-nrfconnect-core
		CONFIGURE_COMMAND ${SUPERUSER_SWITCH} apt-get install -y build-essential python2.7 libudev-dev libgconf-2-4
		BUILD_COMMAND npm install
		INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory ${WORKSPACE_DIR}/tools/nrfconnect_apps
		COMMAND ${CMAKE_COMMAND} -E create_symlink ${WORKSPACE_DIR}/tools/nrfconnect_apps $ENV{HOME}/.nrfconnect-apps/local
		)
endif()

if(DOWNLOAD_NRFCONNECT_PROGRAMMER)
	set(NRFCONNECT_PROGRAMMER_TARGET git_nrfconnect_programmer)
	ExternalProject_Add(git_nrfconnect_programmer
		GIT_REPOSITORY https://github.com/NordicSemiconductor/pc-nrfconnect-programmer
		GIT_CONFIG advice.detachedHead=false
		SOURCE_DIR ${WORKSPACE_DIR}/tools/nrfconnect_apps/pc-nrfconnect-programmer
		BINARY_DIR ${WORKSPACE_DIR}/tools/nrfconnect_apps/pc-nrfconnect-programmer
		CONFIGURE_COMMAND ${SUPERUSER_SWITCH} apt-get install -y build-essential python2.7 libudev-dev libgconf-2-4
		BUILD_COMMAND npm install
		INSTALL_COMMAND ${CMAKE_COMMAND} -E echo "Skipping install step."
		)
endif()

if(DOWNLOAD_CSUTIL)
	message(STATUS "The csutil utility will in the end be replaced by tools in crownstone ble lib")
	ExternalProject_Add(csutil
		GIT_REPOSITORY https://github.com/mrquincle/csutil
		GIT_CONFIG advice.detachedHead=false
		SOURCE_DIR ${WORKSPACE_DIR}/tools/csutil
		BINARY_DIR ${WORKSPACE_DIR}/tools/csutil
		CONFIGURE_COMMAND ${CMAKE_COMMAND} -E echo "Skipping configure step."
		BUILD_COMMAND ${CMAKE_COMMAND} -E echo "Skipping build step."
		INSTALL_COMMAND ${CMAKE_COMMAND} -E echo "Skipping install step."
		DEPENDS ${PYTHON_SETUP_TARGET} ${PYTHON_TEST_VENV_TARGET}
		)
endif()

if(DOWNLOAD_BLUENET_LIB_LOGS)
	set(BLUENET_LIB_LOGS_TARGET bluenet_logs)
	add_custom_target(bluenet_logs
		COMMAND ${CMAKE_COMMAND} -E echo "-- Add bluenet-logs through pip"
		COMMAND ${Python_EXECUTABLE} -m pip -q install wheel bluenet-logs
		WORKING_DIRECTORY ${WORKSPACE_DIR}
		COMMENT "Install bluenet binary logging"
		DEPENDS ${PYTHON_SETUP_TARGET} ${PYTHON_TEST_VENV_TARGET}
		)
endif()

# Dummy target to install all tools that are not required for building and are also not installed
# automatically through ExternalProject_Add such as bluenet_logs.
add_custom_target(tools
	COMMAND ${CMAKE_COMMAND} -E echo "-- Install supporting tools"
	WORKING_DIRECTORY ${WORKSPACE_DIR}
	COMMENT "Install supporting tools"
	DEPENDS ${BLUENET_LIB_LOGS_TARGET} ${NRFUTIL_TARGET}
	)

if(FACTORY_IMAGE)
	MESSAGE(WARNING "Make a factory image. The usual CMakeBuild.config files will not be used. "
		"They will be overwritten by the CMakeFactoryImage.config files. "
		"Make sure that you set -DFACTORY_IMAGE=0 when you continue normal development. ")
	set(FACTORY_IMAGE_HARDWARE_CONFIGURATION_FILE ${WORKSPACE_DIR}/factory-images/hardware/${FACTORY_IMAGE_HARDWARE_CONFIG}/CMakeFactoryImage.config)
	set(FACTORY_IMAGE_SOFTWARE_CONFIGURATION_FILE ${WORKSPACE_DIR}/factory-images/software/${FACTORY_IMAGE_SOFTWARE_CONFIG}/CMakeFactoryImage.config)

	MESSAGE(STATUS "Load hardware configuration file for factory image at ${FACTORY_IMAGE_HARDWARE_CONFIGURATION_FILE}")
	load_configuration("${FACTORY_IMAGE_HARDWARE_CONFIGURATION_FILE}" CMAKE_BLUENET_ARGS)

	MESSAGE(STATUS "Load software configuration file for factory image at ${FACTORY_IMAGE_SOFTWARE_CONFIGURATION_FILE}")
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
	gcc_arm_none_eabi
	)

add_custom_target(target_depend
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
		CMAKE_CACHE_ARGS "${CMAKE_BLUENET_ARGS}"
		CMAKE_ARGS "${CMAKE_BLUENET_ARGS}"
		BUILD_ALWAYS 1
		DEPENDS micro_eec target_depend
		)
endif()

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.config)
if(EXISTS  ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.overwrite.config)
	set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.overwrite.config)
endif()

if(NOT FACTORY_IMAGE)
	add_dependencies(bluenet gcc_arm_none_eabi)
	add_dependencies(bluenet ${NORDIC_SDK_TARGET})
	add_dependencies(bluenet ${MESH_SDK_TARGET})

	if(AUTO_UPDATE_TOOLS)
		if(DOWNLOAD_BLUENET_LIB_LOGS)
			add_dependencies(bluenet ${BLUENET_LIB_LOGS_TARGET})
		endif()
		if(DOWNLOAD_NRFUTIL)
			add_dependencies(bluenet ${NRFUTIL_TARGET})
		endif()
	endif()
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

add_custom_target(write_softdevice
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=WRITE_BINARY" "-DBINARY=${NRF5_DIR}/${SOFTDEVICE_HEX_FILE}" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	#DEPENDS bluenet
	COMMENT "Flashes softdevice to remote target board"
	)

add_custom_target(write_mbr_param_address
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=WRITE" "-DADDRESS=0x10001018" "-DVALUE=0x0007E000" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Writes MBR param page address to UICR"
	)

add_custom_target(read_softdevice_version
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=READ" "-DADDRESS=0x000300C" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Reads version of softdevice"
	)

add_custom_target(erase
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=ERASE" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Erase everything on target board"
	)

add_custom_target(reset
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=RESET" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Reset target board"
	)

#######################################################################################################################
# Forward commands for a particular board towards its build directory
#######################################################################################################################

# Will not be called automatically (this is done through the external project bluenet)
add_custom_target(${BOARD_TARGET}
	COMMAND make
	WORKING_DIRECTORY ${BOARD_TARGET}
	DEPENDS
	bluenet
	COMMENT "Build application"
	)

add_custom_target(debug_application
	COMMAND make debug_application
	WORKING_DIRECTORY ${BOARD_TARGET}
	DEPENDS
	bluenet
	COMMENT "Debug application"
	)

add_custom_target(write_board_version
	COMMAND make write_board_version
	WORKING_DIRECTORY ${BOARD_TARGET}
	DEPENDS
	nrfjprog
	bluenet
	COMMENT "Write board version"
	)

add_custom_target(write_application
	COMMAND make write_application
	WORKING_DIRECTORY ${BOARD_TARGET}
	DEPENDS
	nrfjprog
	bluenet
	COMMENT "Write application"
	)

add_custom_target(write_bootloader
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
add_custom_target(create_git_release
	COMMAND ${CMAKE_COMMAND} -DDEFAULT_MODULES_PATH=${DEFAULT_MODULES_PATH} -DWORKSPACE_DIR=${WORKSPACE_DIR} -P ${DEFAULT_MODULES_PATH}/release.cmake
	COMMENT "Create git release"
	)

add_custom_target(create_git_release_bootloader
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
	DEPENDS ${NRFCONNECT_CORE_TARGET}
	COMMENT "Run nrf connect core development"
	)

add_custom_target(nrfconnect_core
	COMMAND ${CMAKE_COMMAND} -E echo "Run in separate shell. This is an electron app, debug with Ctrl+Shift+I"
	COMMAND npm run app
	WORKING_DIRECTORY ${WORKSPACE_DIR}/tools/nrfconnect/pc-nrfconnect-core
	DEPENDS ${NRFCONNECT_CORE_TARGET}
	COMMENT "Run nrf connect core"
	)

add_custom_target(nrfconnect_programmer
	COMMAND npm run app
	WORKING_DIRECTORY ${WORKSPACE_DIR}/tools/nrfconnect_apps/pc-nrfconnect-programmer
	DEPENDS ${NRFCONNECT_PROGRAMMER_TARGET}
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
	add_custom_target(generate_factory_image ALL
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
	ExternalProject_Add(bluenet-host
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

