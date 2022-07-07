#######################################################################################################################
# The bluenet build systems uses CMake.
#
# Author:    Anne C. van Rossum
# Date:      Oct 28, 2013
# License:   Lesser General Public License (LGPL GNU v3), the Apache License, or the MIT license.
#
# Copyright © 2013 Crownstone
#######################################################################################################################

cmake_minimum_required(VERSION 3.13)

if(COMMAND cmake_policy)
	# only interpret arguments as variables when unquoted
	cmake_policy(SET CMP0054 NEW)
endif()

# Set application shorthand
set(APPLICATION_SHORTHAND "crownstone")

# Set name of project
set(PROJECT_NAME ${APPLICATION_SHORTHAND})

# Set the main file to use for this application
set(MAIN_FILE "cs_main.cpp")

# The directory with additional CMake files
set(DEFAULT_CONF_CMAKE_PATH ${CMAKE_SOURCE_DIR}/conf/cmake)
set(DEFAULT_CONF_GDB_PATH ${CMAKE_SOURCE_DIR}/conf/gdb)

set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${DEFAULT_CONF_CMAKE_PATH}/override_cxx.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_C ${DEFAULT_CONF_CMAKE_PATH}/override_c.cmake)

# Start a project
project(${PROJECT_NAME})

# Other search directories for cmake
set(DEFAULT_MODULES_PATH ${DEFAULT_CONF_CMAKE_PATH}/modules)
set(THIRD_MODULES_PATH ${DEFAULT_CONF_CMAKE_PATH}/modules/third)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CONF_CMAKE_PATH};${DEFAULT_MODULES_PATH};${THIRD_MODULES_PATH}")

message(STATUS "Search for FindX files in ${CMAKE_MODULE_PATH}")

# Get compilation day, this will be used in the binary itself
string(TIMESTAMP COMPILATION_DAY "%Y-%m-%d")

# Get git info, this will be used in the binary in Debug build system
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)

message(STATUS "Git branch ${GIT_REFSPEC} at hash ${GIT_SHA1}")

#######################################################################################################################

if(NOT CONFIG_DIR)
	set(CONFIG_DIR "config")
endif()

if(NOT GDB_PORT)
	set(GDB_PORT "2331")
endif()

if(NOT BOARD_TARGET)
	set(BOARD_TARGET "default")
endif()

set(VERBOSITY                                "4"           CACHE STRING "Verbosity")
set(NRF5_DIR                                 "unset"       CACHE STRING "The nRF5 sdk directory")
set(WORKSPACE_DIR                            "unset"       CACHE STRING "The workspace directory")
set(SOFTDEVICE_HEX_FILE                      ""            CACHE STRING "Softdevice hex file")
set(PASS_FILE                                "crownstone/firmware/dfu_pkg_signing_key" CACHE STRING "Pass file (containing key")
set(KEY_FILE                                 "temp.key"    CACHE STRING "Pass file (containing key")
set(SUPERUSER_SWITCH                         "sudo"        CACHE STRING "Switch to superuser")

# Actually the deployment should not contain this information
math(EXPR SWO_PORT       "${GDB_PORT}+1")
math(EXPR TELNET_PORT    "${GDB_PORT}+2")
math(EXPR RTT_PORT       "${GDB_PORT}+16690")

#######################################################################################################################
# Show variables
#######################################################################################################################

message(STATUS "Hardware board: ${HARDWARE_BOARD}")

message(STATUS "Set verbosity to level: ${VERBOSITY}")
message(STATUS "Compiler: ${CMAKE_C_COMPILER}")

#######################################################################################################################
# Include your own FindX cmake files and options in the following file!
#######################################################################################################################

#######################################################################################################################

message(STATUS "Do not set application start address")
#message(STATUS "Set application start address in linker script to ${APPLICATION_START_ADDRESS}")
#CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/conf/nRF-softdevice.ld.in ${CMAKE_SOURCE_DIR}/conf/nRF-softdevice.ld)

# Libraries (math, stdc++ libraries, etc.) 
# Note! We CANNOT use the size-constrained *_s versions! anymore
# We don't seem to use any libraries anymore... except for including everything that comes with -std=c+11
#set(LIBS "-lc ${LIBRARY_RUNTIME} ${LIBRARY_MATH} -lstdc++")
set(LIBS "")

# Some debug information
if(VERBOSITY GREATER 4)
	message(STATUS "\"${PROJECT_NAME}\" uses the following CXX flags: \"${CMAKE_CXX_FLAGS}\" (can be empty)")
	message(STATUS "Linked libraries: ${LIBS}")
endif()

# Source directory
set(SOURCE_DIR "src")

# Get all includes from nordic
include(nordic.src)

set(BOOTLOADER "_bootloader_.hex")

#######################################################################################################################
# Include source files
#######################################################################################################################

include(crownstone.src)
include(crownstone-application.src)
include(crownstone-platform.src)

list(APPEND FOLDER_SOURCE "${SOURCE_DIR}/${MAIN_FILE}")

if(VERBOSITY GREATER 4)
	foreach(crownstone_file ${FOLDER_SOURCE})
		message(STATUS "Include Crownstone file: " ${crownstone_file})
	endforeach()
endif()

# Be more strict about our own files (then about Nordic's files)
set(CROWNSTONE_ONLY_FLAGS "-W -Wno-unused-parameter")
set_source_files_properties(${FOLDER_SOURCE}
	PROPERTIES COMPILE_FLAGS ${CROWNSTONE_ONLY_FLAGS}
)

# This will add the required nRF5 code files to ${NORDIC_SOURCE}
include(nrf5.src)

if(BUILD_MESHING)
	# This will add the required mesh code files to ${NORDIC_SOURCE}
	include(mesh.src)
else()
	message(STATUS "Mesh SDK files not being compiled (BUILD_MESHING=OFF)")
endif()

if(VERBOSITY GREATER 4)
	foreach(nordic_file ${NORDIC_SOURCE})
		message(STATUS "Include Nordic file: " ${nordic_file})
	endforeach()
endif()

# We now have a variable FOLDER_SOURCE with all source files
list(APPEND FOLDER_SOURCE ${NORDIC_SOURCE})

# For Visual Studio and perhaps other IDEs group the files
source_group("Source files" FILES ${FOLDER_SOURCE})
source_group("Header files" FILES ${FOLDER_HEADER})

if(DEFINED SOFTDEVICE_DIR AND DEFINED SOFTDEVICE_DIR_API)
	include_directories(SYSTEM "${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_API}")
endif()

# Show which directories are actually included to the user
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY include_directories)
if(VERBOSITY GREATER 4)
	foreach(dir ${dirs})
		message(STATUS "Add to the include directories: '${dir}'")
	endforeach()
endif()

if(NOT(DEFINED SOFTDEVICE_DIR AND DEFINED SOFTDEVICE_DIR_HEX))
	set(${SOFTDEVICE_DIR}, "${NRF5_DIR}/components/softdevice/s${SOFTDEVICE_SERIES}/")
	set(${SOFTDEVICE_DIR_HEX}, "hex")
endif()

if(FOLDER_SOURCE STREQUAL "")
	message(FATAL_ERROR "No source code files found. Please add something")
endif()

#######################################################################################################################
# Get version info
#######################################################################################################################

# Get firmware and bootloader version
include(${DEFAULT_MODULES_PATH}/get_versions.cmake)
get_version_info("${WORKSPACE_DIR}/source" FIRMWARE_VERSION FIRMWARE_DFU_VERSION FIRMWARE_RC_VERSION)
get_version_info("${WORKSPACE_DIR}/source/bootloader" BOOTLOADER_VERSION BOOTLOADER_DFU_VERSION BOOTLOADER_RC_VERSION)

# Create bootloader version as integers (DFU is already an integer, RC is treated separately, it's a string)
string(REPLACE "-" ";" BOOTLOADER_WITHOUT_RC_LIST "${BOOTLOADER_VERSION}")
list(GET BOOTLOADER_WITHOUT_RC_LIST 0 BOOTLOADER_VERSION_WITHOUT_RC)
string(REPLACE "." ";" BOOTLOADER_VERSION_LIST "${BOOTLOADER_VERSION_WITHOUT_RC}")
list(GET BOOTLOADER_VERSION_LIST 0 BOOTLOADER_VERSION_MAJOR)
list(GET BOOTLOADER_VERSION_LIST 1 BOOTLOADER_VERSION_MINOR)
list(GET BOOTLOADER_VERSION_LIST 2 BOOTLOADER_VERSION_PATCH)

# Print so we can check correctness
string(CONCAT PRINT_BOOTLOADER_VERSION "${BOOTLOADER_VERSION_MAJOR}."
	"${BOOTLOADER_VERSION_MINOR}."
	"${BOOTLOADER_VERSION_PATCH}"
	)
if(BOOTLOADER_RC_VERSION)
	string(REGEX MATCHALL "[0-9]+" BOOTLOADER_VERSION_PRERELEASE "${BOOTLOADER_RC_VERSION}")
	string(CONCAT PRINT_BOOTLOADER_VERSION "${BOOTLOADER_RC_VERSION}-" "${BOOTLOADER_VERSION_PRERELEASE}")
else()
	set(BOOTLOADER_VERSION_PRERELEASE 255)
endif()
message(STATUS "Bootloader version: ${PRINT_BOOTLOADER_VERSION}")

#######################################################################################################################
# Configure bootloader
#######################################################################################################################

message(STATUS "Descend into bootloader directory and compile if needed")
add_subdirectory(bootloader)

#######################################################################################################################
# Configure bluenet
#######################################################################################################################

set(DEFAULT_CONFIG_FILE "${CMAKE_SOURCE_DIR}/conf/cmake/CMakeBuild.config.default")
set(TARGET_CONFIG_FILE "${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.config")
set(TARGET_CONFIG_OVERWRITE_FILE "${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.overwrite.config")

if(NOT EXISTS "${TARGET_CONFIG_FILE}")
	message(FATAL_ERROR "There should be a CMakeBuild.config file in the ${CONFIG_DIR}/${BOARD_TARGET} directory")
endif()

set_source_files_properties(${NORDIC_SOURCE}
	PROPERTIES COMPILE_FLAGS "-Wno-unused-function -Wno-unknown-pragmas"
)

set_source_files_properties(${FOLDER_SOURCE}
	PROPERTIES OBJECT_DEPENDS ${TARGET_CONFIG_FILE}
)
if(EXISTS "${TARGET_CONFIG_OVERWRITE_FILE}")
	set_source_files_properties(${FOLDER_SOURCE}
		PROPERTIES OBJECT_DEPENDS ${TARGET_CONFIG_OVERWRITE_FILE}
	)
endif()

# calculate some values that are to be used in the auto-generated files below

# auto-generate header files

message(STATUS "Generate cs_MemoryLayout.h")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/shared/cs_MemoryLayout.h.in" "${CMAKE_CURRENT_BINARY_DIR}/shared/cs_MemoryLayout.h")

message(STATUS "Configure cs_StaticConfig.h file")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/cfg/cs_StaticConfig.h.in" "${CMAKE_CURRENT_BINARY_DIR}/include/cfg/cs_StaticConfig.h")

message(STATUS "Configure cs_config.h file")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/third/nrf/cs_config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/include/third/nrf/cs_config.h")

# auto-generate source files

message(STATUS "Configure cs_Git.cpp file")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/cfg/cs_Git.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/src/cfg/cs_Git.cpp")
list(APPEND GENERATED_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/src/cfg/cs_Git.cpp")

message(STATUS "Configure cs_AutoConfig.cpp file")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/cfg/cs_AutoConfig.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/src/cfg/cs_AutoConfig.cpp")
list(APPEND GENERATED_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/src/cfg/cs_AutoConfig.cpp")

message(STATUS "Configure cs_SharedConfig.c file")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/shared/cs_SharedConfig.c.in" "${CMAKE_CURRENT_BINARY_DIR}/shared/cs_SharedConfig.c")
list(APPEND GENERATED_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/shared/cs_SharedConfig.c")

# auto-generate linker files

message(STATUS "Configure nrf_symbols.ld file")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/third/nrf/nrf_symbols.ld.in" "${CMAKE_CURRENT_BINARY_DIR}/include/third/nrf/nrf_symbols.ld")
list(APPEND GENERATED_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/include/third/nrf/nrf_symbols.ld")

link_directories(${CMAKE_CURRENT_BINARY_DIR}/include/third/nrf)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/include/third/nrf/sdk${NORDIC_SDK_VERSION_FULL})

math(EXPR FLASH_MICROAPP_LENGTH "${FLASH_MICROAPP_PAGES} * 1024" OUTPUT_FORMAT HEXADECIMAL)

set(TEMPLATE_DIR ${WORKSPACE_DIR}/scripts/templates)
message(STATUS "Generate build-status.txt script")
configure_file(${TEMPLATE_DIR}/build-status.txt.in ${CMAKE_BINARY_DIR}/build-status.txt)

message(STATUS "RAM for IPC: ${RAM_BLUENET_IPC_LENGTH}")

add_executable(${PROJECT_NAME} ${FOLDER_SOURCE} ${GENERATED_SOURCES} ${FOLDER_HEADER} ${OBJECT_FILES} ${PROJECT_NAME}.bin ${PROJECT_NAME}.hex ${PROJECT_NAME}.elf ${TARGET_CONFIG_FILE})

# Add path with files like sdk_config.h to include directories
# The include/third/nrf file dir is required for files in the sdk (e.g. link "sdk_config.h" without path prefix)
# The ${CMAKE_CURRENT_BINARY_DIR}/include dir is required for files generated through configure_file
set(THIRD_PARTY_NRF_INCLUDE_PATH "include/third/nrf")
message(STATUS "Include path: ${THIRD_PARTY_NRF_INCLUDE_PATH}")
set(THIRD_PARTY_NRF_SDK_INCLUDE_PATH "include/third/nrf/sdk${NORDIC_SDK_VERSION_FULL}")
message(STATUS "Include path: ${THIRD_PARTY_NRF_SDK_INCLUDE_PATH}")
target_include_directories(${PROJECT_NAME} PRIVATE "${THIRD_PARTY_NRF_INCLUDE_PATH}" "${THIRD_PARTY_NRF_SDK_INCLUDE_PATH}" "${CMAKE_CURRENT_BINARY_DIR}/include/third/nrf" "${CMAKE_CURRENT_BINARY_DIR}/include""${CMAKE_CURRENT_BINARY_DIR}/shared")

# Save intermediate files, these are used for binary logging.
target_compile_options(${PROJECT_NAME} PRIVATE -save-temps=obj)

# This does lead to a cmake reconfigure when the file changes... However, nothing else gets rebuild after
# the reconfiguration
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${TARGET_CONFIG_FILE})
if(EXISTS ${TARGET_CONFIG_OVERWRITE_FILE})
	set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${TARGET_CONFIG_OVERWRITE_FILE})
endif()

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${WORKSPACE_DIR}/source/VERSION)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${WORKSPACE_DIR}/source/bootloader/VERSION)

# Reconfigure cmake to get up-to-date GIT_SHA1
set_property(GLOBAL APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/.git/index")

# Make also depend on helper files
set_property(GLOBAL APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/conf/cmake/crownstone.defs.cmake")
set_property(GLOBAL APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/conf/cmake/crownstone.src.cmake")

set(INCLUDE_ONLY_SECTIONS "")

# https://stackoverflow.com/questions/5278444/adding-a-custom-command-with-the-file-name-as-a-target
add_custom_command(OUTPUT ${PROJECT_NAME}.bin
	COMMAND ${CMAKE_OBJCOPY_OVERLOAD} ${INCLUDE_ONLY_SECTIONS} -O binary ${PROJECT_NAME} ${PROJECT_NAME}.bin
	COMMAND stat -t ${PROJECT_NAME}.bin | cut -f2 -d ' ' | xargs printf "** Firmware size : %s bytes"
	DEPENDS
		display_build_status
		${PROJECT_NAME}
	COMMENT "Object copy ${PROJECT_NAME} to ${PROJECT_NAME}.bin")

add_custom_command(OUTPUT ${PROJECT_NAME}.hex
	COMMAND ${CMAKE_OBJCOPY_OVERLOAD} ${INCLUDE_ONLY_SECTIONS} -O ihex ${PROJECT_NAME} ${PROJECT_NAME}.hex
	DEPENDS
		${PROJECT_NAME}
	COMMENT "Object copy ${PROJECT_NAME} to ${PROJECT_NAME}.hex")

add_custom_command(OUTPUT ${PROJECT_NAME}.elf
	COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_NAME} ${PROJECT_NAME}.elf
	COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
	COMMAND ${CMAKE_COMMAND} -DDEFAULT_MODULES_PATH=${DEFAULT_MODULES_PATH} -DCMAKE_SIZE=${CMAKE_SIZE} -DBINARY=${PROJECT_NAME}.elf -DOFFSET=${APPLICATION_START_ADDRESS} -DMAX_SIZE=${FLASH_MICROAPP_BASE} -P ${DEFAULT_MODULES_PATH}/check_sizes.cmake
	DEPENDS
		${PROJECT_NAME}
	COMMENT "Copy ${PROJECT_NAME} to ${PROJECT_NAME}.elf")

add_custom_target(generate_dat
	COMMAND echo
	COMMAND echo "** Use srec_cat to create the INIT .dat file for the DFU"
	COMMAND echo
	COMMAND echo "srec_cat ${PROJECT_NAME}.hex -intel -exclude 0x3FC00 0x3FC20 -generate 0x3FC00 0x3FC04 -l-e-constant 0x01 4 -generate 0x3FC04 0x3FC08 -l-e-constant 0x00 4 -generate 0x3FC08 0x3FC0C -l-e-constant 0xFE 4 -generate 0x3FC0C 0x3FC20 -constant 0x00 -o ${PROJECT_NAME}.dat -intel"
	COMMAND srec_cat ${PROJECT_NAME}.hex -intel -exclude 0x3FC00 0x3FC20 -generate 0x3FC00 0x3FC04 -l-e-constant 0x01 4 -generate 0x3FC04 0x3FC08 -l-e-constant 0x00 4 -generate 0x3FC08 0x3FC0C -l-e-constant 0xFE 4 -generate 0x3FC0C 0x3FC20 -constant 0x00 -o ${PROJECT_NAME}.dat -intel
	DEPENDS ${PROJECT_NAME}.hex
	COMMENT "Generate the INIT .dat file for the DFU"
	)

add_custom_target(merge_all
	COMMAND echo
	COMMAND echo "** Use srec_cat to create one merged file"
	COMMAND echo "** Includes softdevice, bootloader, firmware, bootloader settings, and all UICR fields, except board version."
	COMMAND echo
	COMMAND echo "srec_cat ${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_HEX}/${SOFTDEVICE}_softdevice.hex -intel bootloader.hex -intel ${PROJECT_NAME}.hex -intel bootloader-settings.hex -intel -exclude 0x10001014 0x10001018 -generate 0x10001014 0x10001018 -l-e-constant $BOOTLOADER_START_ADDRESS 4 -exclude 0x10001018 0x1000101C -generate 0x10001018 0x1000101C -l-e-constant 0x0007E000 4 -o combined.hex -intel"
	COMMAND srec_cat ${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_HEX}/${SOFTDEVICE}_softdevice.hex -intel bootloader.hex -intel ${PROJECT_NAME}.hex -intel bootloader-settings.hex -intel -exclude 0x10001014 0x10001018 -generate 0x10001014 0x10001018 -l-e-constant $BOOTLOADER_START_ADDRESS 4 -exclude 0x10001018 0x1000101C -generate 0x10001018 0x1000101C -l-e-constant 0x0007E000 4 -o combined.hex -intel
	DEPENDS ${PROJECT_NAME}.bin
	COMMENT "Merge all but board version into one hex"
	)

add_custom_target(analyze
	COMMAND echo
	COMMAND echo "** Show headers"
	COMMAND ${CMAKE_READELF} --headers ${PROJECT_NAME}.elf
	#COMMAND ${CMAKE_OBJDUMP} -h ${PROJECT_NAME}.elf
	COMMAND echo
	COMMAND echo "** Show largest symbols / functions"
	COMMAND ${CMAKE_NM} --print-size --size-sort --radix=d ${PROJECT_NAME}.elf
	COMMAND echo
	COMMAND echo "** Show size of sections"
	COMMAND ${CMAKE_SIZE} -A -d ${PROJECT_NAME}.elf
	DEPENDS ${PROJECT_NAME}.elf
	COMMENT "Read headers ${PROJECT_NAME}.elf"
	)

add_custom_target(display_build_status
	COMMAND ${CMAKE_COMMAND} -E echo "** Display build status"
	COMMAND cat ${CMAKE_BINARY_DIR}/build-status.txt
	COMMENT "Display build status"
)

# The reason to - again - write all those parameters is that at runtime they are not available. If we want
# them to be available we will have to send them as arguments like this.
# TODO: Rename CONFIG_FILE here to TARGET_RUNTIME_CONFIG_FILE
set(DEFAULT_TOOL_PARAM "-DDEFAULT_MODULES_PATH:STRING=${DEFAULT_MODULES_PATH}\
	-DPROJECT_NAME:STRING=${PROJECT_NAME}\
	-DWORKSPACE_DIR:STRING=${WORKSPACE_DIR}\
	-DDEFAULT_CONFIG_FILE:STRING=${DEFAULT_CONFIG_FILE}\
	-DTARGET_CONFIG_FILE:STRING=${TARGET_CONFIG_FILE}\
	-DTARGET_CONFIG_OVERWRITE_FILE:STRING=${TARGET_CONFIG_OVERWRITE_FILE}\
	-DCONFIG_FILE:STRING=${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.runtime.config\
	-DDEFAULT_CONF_GDB_PATH:STRING=${DEFAULT_CONF_GDB_PATH}\
	-DCOMPILER_PATH:STRING=${COMPILER_PATH}\
	-DKEYS_JSON_FILE:STRING=${KEYS_JSON_FILE}\
	-DDEVICE:STRING=${DEVICE}\
	-DNRF_DEVICE_FAMILY:STRING=${NRF_DEVICE_FAMILY}\
	-DSERIAL_NUM:STRING=${SERIAL_NUM}\
	-DGDB_PORT:STRING=${GDB_PORT}\
	-DSWO_PORT:STRING=${SWO_PORT}\
	-DTELNET_PORT:STRING=${TELNET_PORT}\
	-DUART_DEVICE:STRING=${UART_DEVICE}\
	-DUART_BAUDRATE:STRING=${UART_BAUDRATE}\
	-DRTT_PORT:STRING=${RTT_PORT}")

# An internal target that updates scripts with (even) runtime configuration values
add_custom_target(write_runtime_config
	COMMAND ${CMAKE_COMMAND} -E echo "** Write runtime config"
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} -P ${DEFAULT_MODULES_PATH}/writescripts.cmake
	COMMENT "Write runtime config"
)



add_custom_target(debug_server
	COMMAND ${CMAKE_COMMAND} -E echo "** Debug server, keep running in separate shell"
	COMMAND ./gdb-server.sh
	DEPENDS write_runtime_config
	COMMENT "Debug server, runs in background"
	)

add_custom_target(uart_client
	COMMAND ${CMAKE_COMMAND} -E echo "** UART client, keep running in separate shell"
	COMMAND ./minicom.sh
	DEPENDS write_runtime_config
	COMMENT "UART client, runs in background"
	)

add_custom_target(uart_binary_client
	COMMAND ${CMAKE_COMMAND} -E echo "** UART binary client, keep running in separate shell"
	COMMAND ./log-client.sh
	DEPENDS write_runtime_config
	COMMENT "UART client, runs in background"
	)

add_custom_target(uart_combined_client
	COMMAND ${CMAKE_COMMAND} -E echo "** UART plaintext and binary client, keep running in separate shell"
	COMMAND ./log-client.sh -p
	DEPENDS write_runtime_config
	COMMENT "UART client, runs in background"
	)

add_custom_target(rtt_client
	COMMAND ${CMAKE_COMMAND} -E echo "** RTT client, keep running in separate shell"
	COMMAND ./rtt-client.sh
	DEPENDS write_runtime_config
	COMMENT "RTT client, runs in background"
	)

add_custom_target(extract_logs
	COMMAND ${CMAKE_COMMAND} -E echo "** Extract logs"
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=EXTRACT" -P ${DEFAULT_MODULES_PATH}/extract_logs.cmake
	COMMENT "Extract logs from preprocessed source files."
	)

add_custom_target(debug_application
	COMMAND ${CMAKE_COMMAND} -E echo "** Debug application"
	COMMAND ./debug-application.sh
	DEPENDS ${PROJECT_NAME}.elf write_runtime_config
	COMMENT "Debug ${PROJECT_NAME}.elf"
	)

add_custom_target(debug_bootloader
	COMMAND ${CMAKE_COMMAND} -E echo "** Debug bootloader"
	COMMAND ./debug-bootloader.sh
	WORKING_DIRECTORY bootloader
	DEPENDS bootloader write_runtime_config
	COMMENT "Debug bootloader.elf"
	)

add_custom_target(write_application
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=WRITE_BINARY" "-DBINARY=${PROJECT_NAME}.hex" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	DEPENDS ${PROJECT_NAME}.hex
	COMMENT "Flashes application to remote target board (takes a while)"
	)

add_custom_target(setup
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=SETUP" -P ${DEFAULT_MODULES_PATH}/cstool.cmake
	COMMENT "Setup Crownstone via csutil"
	)

add_custom_target(factory_reset
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=FACTORY_RESET" -P ${DEFAULT_MODULES_PATH}/cstool.cmake
	COMMAND ${CMAKE_COMMAND} -E echo "** Factory reset"
	COMMAND ./factory-reset.sh
	DEPENDS write_runtime_config
	COMMENT "Perform a factory reset of a Crownstone via csutil"
	)

add_custom_target(memory_usage_test_client
	COMMAND ${WORKSPACE_DIR}/scripts/mem-usage-test.py
	COMMENT "Run memory usage test client, which communicates with a Crownstone."
	)

add_custom_target(check
	COMMAND cppcheck --enable=style --quiet ${CMAKE_SOURCE_DIR}
	COMMENT "Check for style or worse types of errors"
	)

add_custom_target(generate_documentation
	COMMAND ${CMAKE_COMMAND} -E echo "Run doxygen from ${CMAKE_SOURCE_DIR}"
	COMMAND cd ${WORKSPACE_DIR} && doxygen source/doxygen.config
	COMMENT "Run documentation generator"
	)

add_custom_target(view_documentation
	COMMAND ${CMAKE_COMMAND} -E echo "Open an .html viewer through xdg-open"
	COMMAND xdg-open ${WORKSPACE_DIR}/docs/html/index.html
	COMMENT "View documentation"
	DEPENDS generate_documentation
	)

add_custom_target(list_debuggers
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=LIST" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "List debuggers"
	)

add_custom_target(erase
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=ERASE" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Erase everything on target board"
	)

add_custom_target(reset
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=RESET" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Reset target board"
	)
	
message(STATUS "Persistent flags disabled: ${PERSISTENT_FLAGS_DISABLED}")

add_custom_target(read_bootloader_address
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=READ" "-DADDRESS=0x10001014" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Read bootloader address"
	)

add_custom_target(read_mbr_parameter_page_address
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=READ" "-DADDRESS=0x10001018" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Read MBR parameter page address"
	)

add_custom_target(read_nrf_chip_version
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=READ" "-DADDRESS=0x10000100" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=READ" "-DADDRESS=0x10000104" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Read nRF chip version"
	)

add_custom_target(read_board_version
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=READ" "-DADDRESS:STRING=${HARDWARE_BOARD_ADDRESS}" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Read board version"
	)

add_custom_target(read_mac_address
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=READ" "-DADDRESS=0x100000A4" "-DCOUNT=8" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Read MAC address"
	)

add_custom_target(write_config
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} -P ${DEFAULT_MODULES_PATH}/writeconfig.cmake
	COMMMENT "Write config"
	)

# Ideally we only want to call load_hardware_version when the write_hw_version target is used
#if(NOT CONFIG_DIR STREQUAL "release")
set(JLINK_LINUX_DOWNLOAD_URL_BOARD_HARDWARE_VERSION "")
include(${DEFAULT_MODULES_PATH}/load_hardware_version_mapping.cmake)
message(STATUS "Get version for hardware \"${HARDWARE_BOARD}\"")
load_hardware_version(${CMAKE_SOURCE_DIR}/include/cfg/cs_Boards.h ${HARDWARE_BOARD} LOAD_BOARD_HARDWARE_VERSION)
message(STATUS "Got ${LOAD_BOARD_HARDWARE_VERSION}")
include(${DEFAULT_MODULES_PATH}/hex.cmake)
to_hex_raw(${LOAD_BOARD_HARDWARE_VERSION} HARDWARE_BOARD_HEX 8 "0x")
message(STATUS "Hexadecimal ${HARDWARE_BOARD_HEX}")
#endif()

add_custom_target(write_board_version
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=WRITE" "-DADDRESS=${HARDWARE_BOARD_ADDRESS}" "-DVALUE=${LOAD_BOARD_HARDWARE_VERSION}" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Flashes board version to remote target board"
	)

add_custom_target(increment_application_version
	COMMAND ${CMAKE_COMMAND} -E echo "Increment application version in VERSION file"
	COMMAND ./increment-application-version.sh
	COMMENT "Increment application version"
	DEPENDS write_runtime_config
	)
	
set(BOOTLOADER_HEX_FILE "${CMAKE_BINARY_DIR}/bootloader/bootloader_settings.hex")
set(BOOTLOADER_BIN_FILE "${CMAKE_BINARY_DIR}/bootloader/bootloader_settings.bin")

add_custom_target(build_bootloader_settings
	COMMAND ${CMAKE_COMMAND} -E echo "** Firmware version: ${FIRMWARE_VERSION}"
	COMMAND ${CMAKE_COMMAND} -E echo "** Bootloader version: ${BOOTLOADER_VERSION}"
	COMMAND ${CMAKE_COMMAND} -E echo "** Use files in directory: ${CMAKE_BINARY_DIR}"
	COMMAND nrfutil settings generate --family NRF52 --application "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex" --application-version ${FIRMWARE_DFU_VERSION} --bootloader-version ${BOOTLOADER_DFU_VERSION} --bl-settings-version 2 "${BOOTLOADER_HEX_FILE}"
	COMMAND ${CMAKE_OBJCOPY_OVERLOAD} -I ihex ${BOOTLOADER_HEX_FILE} -O binary ${BOOTLOADER_BIN_FILE}
	COMMAND ${CMAKE_COMMAND} -E echo "** Generated file: ${BOOTLOADER_HEX_FILE}"
	COMMENT "Create bootloader settings"
	)

add_custom_target(write_bootloader_settings
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=WRITE_BINARY" "-DBINARY=${BOOTLOADER_HEX_FILE}" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Flashes bootloader settings to remote target board"
	DEPENDS build_bootloader_settings
	)

add_custom_target(read_bootloader_settings
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=READ" "-DADDRESS=${CS_BOOTLOADER_SETTINGS_ADDRESS}" "-DCOUNT=128" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Read bootloader settings from remote target board"
	)

add_custom_target(read_bootloader_settings_backup
	COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=READ" "-DADDRESS=${CS_BOOTLOADER_SETTINGS_BACKUP_ADDRESS}" "-DCOUNT=128" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
	COMMENT "Read bootloader settings from remote target board"
	)

add_custom_target(merge_bootloader_settings
	COMMAND ${CMAKE_COMMAND} -E echo
	COMMAND ${CMAKE_COMMAND} -E echo "** Use mergehex to combine bootloader with bootloader settings"
	COMMAND ${CMAKE_COMMAND} -E echo "      Note that the settings can only be build with info from the application"
	COMMAND ${CMAKE_COMMAND} -E echo
	COMMAND mergehex -m ${CMAKE_BINARY_DIR}/bootloader/bootloader.hex ${CMAKE_BINARY_DIR}/bootloader/bootloader_settings.hex -o ${CMAKE_BINARY_DIR}/bootloader/bootloader_with_settings.hex
	COMMENT "Merges bootloader with bootloader settings"
	)

set(FIRMWARE_DFU_ZIP     crownstone_all_${FIRMWARE_VERSION}.zip)
set(APPLICATION_DFU_ZIP  crownstone_${FIRMWARE_VERSION}.zip)
set(BOOTLOADER_DFU_ZIP   bootloader_${BOOTLOADER_VERSION}.zip)

add_custom_target(extract_sd_id
	COMMAND ${CMAKE_COMMAND} -E echo
	COMMAND ${CMAKE_COMMAND} -E echo "** Extract id from softdevice file: ${SOFTDEVICE_HEX_FILE}"
	COMMAND ${CMAKE_OBJCOPY_OVERLOAD} -I ihex ${NRF5_DIR}/${SOFTDEVICE_HEX_FILE} -O binary check.bin
	COMMAND bash -c "xxd -s 0x300C -l 2 -e -u check.bin | awk '{print \"0x\"$2}' > softdevice_id" VERBATIM
	COMMAND bash -c "echo \"   Softdevice id: $(cat softdevice_id)\"" VERBATIM
	COMMAND ${CMAKE_COMMAND} -E echo
)

add_custom_target(generate_dfu_package_all
	COMMAND ${CMAKE_COMMAND} -E echo
	COMMAND ${CMAKE_COMMAND} -E echo "** Use nrfutil to generate DFU package"
	COMMAND ${CMAKE_COMMAND} -E echo "     Application: ${PROJECT_NAME}.hex"
	COMMAND ${CMAKE_COMMAND} -E echo "     Softdevice:  ${SOFTDEVICE_HEX_FILE}"
	COMMAND ${CMAKE_COMMAND} -E echo "     Bootloader:  bootloader.hex"
	COMMAND ${CMAKE_COMMAND} -E echo
	COMMAND bash -c "echo \"Softdevice id: $(cat softdevice_id)\"" VERBATIM
	COMMAND ${CMAKE_COMMAND} -E echo
	COMMAND ${CMAKE_COMMAND} -E echo "** Use pass to get encrypted key file, using entry: ${PASS_FILE}"
	COMMAND ${CMAKE_COMMAND} -E touch ${KEY_FILE}
	COMMAND pass ${PASS_FILE} > ${KEY_FILE} USES_TERMINAL
	COMMAND ${CMAKE_COMMAND} -E echo "     Temp key file: ${KEY_FILE}"
	COMMAND bash -c "echo 'nrfutil pkg generate --hw-version 52 --sd-req $(cat softdevice_id) --sd-id $(cat softdevice_id) --key-file ${KEY_FILE} --application ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex --application-version ${FIRMWARE_DFU_VERSION} --softdevice ${NRF5_DIR}/${SOFTDEVICE_HEX_FILE} --bootloader ${CMAKE_BINARY_DIR}/bootloader/bootloader.hex --bootloader-version ${BOOTLOADER_DFU_VERSION} all_dfu.zip'" VERBATIM USES_TERMINAL
	COMMAND bash -c "nrfutil pkg generate --hw-version 52 --sd-req $(cat softdevice_id) --sd-id $(cat softdevice_id) --key-file ${KEY_FILE} --application ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex --application-version ${FIRMWARE_DFU_VERSION} --softdevice ${NRF5_DIR}/${SOFTDEVICE_HEX_FILE} --bootloader ${CMAKE_BINARY_DIR}/bootloader/bootloader.hex --bootloader-version ${BOOTLOADER_DFU_VERSION} all_dfu.zip" VERBATIM USES_TERMINAL
	COMMAND ${CMAKE_COMMAND} -E make_directory dfu
	COMMAND ${CMAKE_COMMAND} -E copy all_dfu.zip dfu/${FIRMWARE_DFU_ZIP}
	COMMAND ${CMAKE_COMMAND} -E remove ${KEY_FILE}
	COMMAND bash -c "sha1sum dfu/${FIRMWARE_DFU_ZIP} | cut -f1 -d' ' > dfu/${FIRMWARE_DFU_ZIP}.sha1" VERBATIM
	COMMAND ${CMAKE_COMMAND} -E echo
	COMMENT "Generate DFU package with Softdevice, bootloader, and application"
	DEPENDS extract_sd_id
	)

add_custom_target(generate_dfu_package_application
	COMMAND ${CMAKE_COMMAND} -E echo
	COMMAND ${CMAKE_COMMAND} -E echo "** Use nrfutil to generate DFU package"
	COMMAND ${CMAKE_COMMAND} -E echo "     Application: ${PROJECT_NAME}.hex"
	COMMAND ${CMAKE_COMMAND} -E echo
	COMMAND bash -c "echo \"Softdevice id: $(cat softdevice_id)\"" VERBATIM
	COMMAND ${CMAKE_COMMAND} -E echo
	COMMAND ${CMAKE_COMMAND} -E echo "** Use pass to get encrypted key file, using entry: ${PASS_FILE}"
	COMMAND ${CMAKE_COMMAND} -E touch ${KEY_FILE}
	COMMAND pass ${PASS_FILE} > ${KEY_FILE}
	COMMAND ${CMAKE_COMMAND} -E echo "     Temp key file: ${KEY_FILE}"
	COMMAND bash -c "echo 'nrfutil pkg generate --hw-version 52 --sd-req $(cat softdevice_id) --key-file ${KEY_FILE} --application ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex --application-version ${FIRMWARE_DFU_VERSION} application_dfu.zip'" VERBATIM USES_TERMINAL
	COMMAND bash -c "nrfutil pkg generate --hw-version 52 --sd-req $(cat softdevice_id) --key-file ${KEY_FILE} --application ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex --application-version ${FIRMWARE_DFU_VERSION} application_dfu.zip" VERBATIM USES_TERMINAL
	COMMAND ${CMAKE_COMMAND} -E make_directory dfu
	COMMAND ${CMAKE_COMMAND} -E copy application_dfu.zip dfu/${APPLICATION_DFU_ZIP}
	COMMAND ${CMAKE_COMMAND} -E remove ${KEY_FILE}
	COMMAND bash -c "sha1sum dfu/${APPLICATION_DFU_ZIP} | cut -f1 -d' ' > dfu/${APPLICATION_DFU_ZIP}.sha1" VERBATIM
	COMMAND ${CMAKE_COMMAND} -E echo
	COMMENT "Generate DFU package with application only"
	DEPENDS extract_sd_id
	)

add_custom_target(generate_dfu_package_bootloader
	COMMAND ${CMAKE_COMMAND} -E echo
	COMMAND ${CMAKE_COMMAND} -E echo "** Use nrfutil to generate DFU package"
	COMMAND ${CMAKE_COMMAND} -E echo "     Bootloader:  bootloader.hex"
	COMMAND ${CMAKE_COMMAND} -E echo
	COMMAND bash -c "echo \"Softdevice id: $(cat softdevice_id)\"" VERBATIM
	COMMAND ${CMAKE_COMMAND} -E echo
	COMMAND ${CMAKE_COMMAND} -E echo "** Use pass to get encrypted key file, using entry: ${PASS_FILE}"
	COMMAND ${CMAKE_COMMAND} -E touch ${KEY_FILE}
	COMMAND pass ${PASS_FILE} > ${KEY_FILE}
	COMMAND ${CMAKE_COMMAND} -E echo "     Temp key file: ${KEY_FILE}"
	COMMAND bash -c "echo 'nrfutil pkg generate --hw-version 52 --sd-req $(cat softdevice_id) --key-file ${KEY_FILE} --bootloader ${CMAKE_BINARY_DIR}/bootloader/bootloader.hex --bootloader-version ${BOOTLOADER_DFU_VERSION} bootloader_dfu.zip'" VERBATIM USES_TERMINAL
	COMMAND bash -c "nrfutil pkg generate --hw-version 52 --sd-req $(cat softdevice_id) --key-file ${KEY_FILE} --bootloader ${CMAKE_BINARY_DIR}/bootloader/bootloader.hex --bootloader-version ${BOOTLOADER_DFU_VERSION} bootloader_dfu.zip" VERBATIM USES_TERMINAL
	COMMAND ${CMAKE_COMMAND} -E make_directory dfu
	COMMAND ${CMAKE_COMMAND} -E copy bootloader_dfu.zip dfu/${BOOTLOADER_DFU_ZIP}
	COMMAND ${CMAKE_COMMAND} -E remove ${KEY_FILE}
	COMMAND bash -c "sha1sum dfu/${BOOTLOADER_DFU_ZIP} | cut -f1 -d' ' > dfu/${BOOTLOADER_DFU_ZIP}.sha1" VERBATIM
	COMMAND ${CMAKE_COMMAND} -E echo
	COMMENT "Generate DFU package with bootloader only"
	DEPENDS extract_sd_id
	)



if(NOT DEFINED FIRMWARE_RC_VERSION OR "${FIRMWARE_RC_VERSION}" STREQUAL "")
	set(RELEASE_REPOSITORY_DIRECTORY "${RELEASE_REPOSITORY}/firmwares/crownstone_${FIRMWARE_VERSION}")
else()
	set(RELEASE_REPOSITORY_DIRECTORY "${RELEASE_CANDIDATE_REPOSITORY}/firmwares/crownstone_${FIRMWARE_VERSION}")
endif()


if(EXISTS ${RELEASE_REPOSITORY_DIRECTORY} )
	message(STATUS "Already released... No create_release_in_repository target available")
else()

	add_custom_target(create_release_in_repository
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "** Copy to release repository \"${RELEASE_REPOSITORY_DIRECTORY}\""
			COMMAND ${CMAKE_COMMAND} -E make_directory ${RELEASE_REPOSITORY_DIRECTORY}/bin
			COMMAND ${CMAKE_COMMAND} -E echo "   Copy from ${CMAKE_BINARY_DIR}"
			COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/dfu/crownstone*.zip ${RELEASE_REPOSITORY_DIRECTORY}/bin
			COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/dfu/crownstone*.zip.sha1 ${RELEASE_REPOSITORY_DIRECTORY}/bin
			COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/bootloader/bootloader_settings.hex ${RELEASE_REPOSITORY_DIRECTORY}/bin/bootloader-settings.hex
			COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/crownstone.elf ${RELEASE_REPOSITORY_DIRECTORY}/bin
			COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/crownstone.hex ${RELEASE_REPOSITORY_DIRECTORY}/bin
			COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/crownstone.bin ${RELEASE_REPOSITORY_DIRECTORY}/bin
			COMMAND ${CMAKE_COMMAND} -E copy ${NRF5_DIR}/${SOFTDEVICE_HEX_FILE} ${RELEASE_REPOSITORY_DIRECTORY}/bin/softdevice.hex
			COMMAND ${CMAKE_COMMAND} -E echo "   Copy release config file from ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}"
			COMMAND ${CMAKE_COMMAND} -E make_directory ${RELEASE_REPOSITORY_DIRECTORY}/config
			COMMAND ${CMAKE_COMMAND} -E copy ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.config ${RELEASE_REPOSITORY_DIRECTORY}/config
			COMMENT "Copy all generated files to release repository"
			)
endif()

if(NOT DEFINED BOOTLOADER_RC_VERSION OR "${BOOTLOADER_RC_VERSION}" STREQUAL "")
	set(RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER "${RELEASE_REPOSITORY}/bootloaders/bootloader_${BOOTLOADER_VERSION}")
else()
	set(RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER "${RELEASE_CANDIDATE_REPOSITORY}/bootloaders/bootloader_${BOOTLOADER_VERSION}")
endif()


if(EXISTS ${RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER} )
	message(STATUS "Already released... No create_bootloader_release_in_repository target available")
else()

	add_custom_target(create_bootloader_release_in_repository
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "** Copy to release repository \"${RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER}\""
			COMMAND ${CMAKE_COMMAND} -E make_directory ${RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER}/bin
			COMMAND ${CMAKE_COMMAND} -E echo "   Copy from ${CMAKE_BINARY_DIR}"
			COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/dfu/bootloader*.zip ${RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER}/bin
			COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/dfu/bootloader*.zip.sha1 ${RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER}/bin
			COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/bootloader/bootloader.elf ${RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER}/bin
			COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/bootloader/bootloader.hex ${RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER}/bin
			COMMAND ${CMAKE_COMMAND} -E copy ${NRF5_DIR}/${SOFTDEVICE_HEX_FILE} ${RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER}/bin/softdevice.hex
			COMMAND ${CMAKE_COMMAND} -E echo "   Copy release config file from ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}"
			COMMAND ${CMAKE_COMMAND} -E make_directory ${RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER}/config
			COMMAND ${CMAKE_COMMAND} -E copy ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.config ${RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER}/config
			COMMENT "Copy all generated files to release repository"
			)
endif()

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${PROJECT_NAME}.bin)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${PROJECT_NAME}.elf)

set(FILE_MEMORY_LAYOUT "-Tgeneric_gcc_nrf52.ld")
set(PATH_FILE_MEMORY "-L${CMAKE_SOURCE_DIR}/include/third/nrf/")
set(PATH_FILE_MEMORY "${PATH_FILE_MEMORY} -L${NRF5_DIR}/modules/nrfx/mdk/")
set(CMAKE_EXE_LINKER_FLAGS "${PATH_FILE_MEMORY} ${FILE_MEMORY_LAYOUT} ${CMAKE_EXE_LINKER_FLAGS}")
target_link_libraries(${PROJECT_NAME} ${LIBS})

# We actually do not need the file without extension
#install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)

# TARGETS are not accepted from other directoreis
install(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin" DESTINATION .)
install(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf" DESTINATION .)
install(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex" DESTINATION .)
install(FILES "${CMAKE_BINARY_DIR}/bootloader/bootloader.elf" DESTINATION .)
install(FILES "${CMAKE_BINARY_DIR}/bootloader/bootloader.hex" DESTINATION .)
if(EXISTS "${CMAKE_BINARY_DIR}/bootloader/bootloader_settings.hex")
	install(FILES "${CMAKE_BINARY_DIR}/bootloader/bootloader_settings.hex" DESTINATION .)
endif()
install(FILES "${NRF5_DIR}/${SOFTDEVICE_HEX_FILE}" DESTINATION .)
install(DIRECTORY "${CMAKE_BINARY_DIR}/dfu"       DESTINATION . FILES_MATCHING PATTERN "*.zip" PATTERN "*.sha1")
	
set(FILE_LIST_VARIABLES "${CMAKE_BINARY_DIR}/list_variables.txt")
file(WRITE ${FILE_LIST_VARIABLES} "")
message(STATUS "Write variables to ${FILE_LIST_VARIABLES}")
get_cmake_property(_variableNames VARIABLES)
list(SORT _variableNames)
foreach(_variableName ${_variableNames})
	file(APPEND "${FILE_LIST_VARIABLES}" "${_variableName}=${${_variableName}}\n")
endforeach()

