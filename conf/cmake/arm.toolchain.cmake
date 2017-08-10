#######################################################################################################################
# The build systems uses CMake. All the automatically generated code falls under the Lesser General Public License
# (LGPL GNU v3), the Apache License, or the MIT license, your choice.
#
# Author:	 Crownstone Team
# Date: 	 Oct 28, 2013
#
# Copyright © 2013 Crownstone B.V. <team@crownstone.rocks>
#######################################################################################################################
#Check http://www.cmake.org/Wiki/CMake_Cross_Compiling
#
# Note that the arm.toolchain.cmake is run BEFORE the variables of the CMakeLists.txt are set!

# Set to Generic, or tests will fail (they will fail anyway)
SET(CMAKE_SYSTEM_NAME Generic)

# Tell we want to cross-compile
SET(CMAKE_SYSTEM_VERSION 1)
SET(CMAKE_SYSTEM_PROCESSOR arm)
SET(CMAKE_CROSSCOMPILING 1)

# Load compiler options from the configuration file. This is done through CMakeConfig.cmake which on its turn will 
# get the configuration data from the proper CMakeBuild.config file from some configuration directory.
SET(CONFIG_FILE ${CMAKE_SOURCE_DIR}/CMakeConfig.cmake)
IF(EXISTS "${CONFIG_FILE}")
	MESSAGE(STATUS "Load config file ${CONFIG_FILE}")
	INCLUDE(${CONFIG_FILE})
ELSE()
	MESSAGE(WARNING "Cannot find config file ${CONFIG_FILE}")
ENDIF()

MESSAGE(STATUS "ARM cmake: Bluetooth name ${BLUETOOTH_NAME}")

#######################################################################################################################
# type of compiler we want to use, the COMPILER_TYPE can be empty if normal gcc and g++ compilers are intended
SET(COMPILER_TYPE_PREFIX ${COMPILER_TYPE})

#######################################################################################################################

# The extension .obj is just ugly, set it back to .o (does not work)
SET(CMAKE_CXX_OUTPUT_EXTENSION .o)

# Make cross-compiler easy to find (but we will use absolute paths anyway)
SET(PATH "${PATH}:${COMPILER_PATH}/bin")
MESSAGE(STATUS "ARM cmake: PATH is set to: ${PATH}")

# Specify the cross compiler, linker, etc.
SET(CMAKE_C_COMPILER                   ${COMPILER_PATH}/bin/${COMPILER_TYPE}gcc)
SET(CMAKE_CXX_COMPILER                 ${COMPILER_PATH}/bin/${COMPILER_TYPE}g++)
SET(CMAKE_ASM                          ${COMPILER_PATH}/bin/${COMPILER_TYPE}as)
SET(CMAKE_LINKER                       ${COMPILER_PATH}/bin/${COMPILER_TYPE}ld)
SET(CMAKE_READELF                      ${COMPILER_PATH}/bin/${COMPILER_TYPE}readelf)
SET(CMAKE_OBJDUMP                      ${COMPILER_PATH}/bin/${COMPILER_TYPE}objdump)
SET(CMAKE_OBJCOPY                      ${COMPILER_PATH}/bin/${COMPILER_TYPE}objcopy)
SET(CMAKE_SIZE                         ${COMPILER_PATH}/bin/${COMPILER_TYPE}size)
SET(CMAKE_NM                           ${COMPILER_PATH}/bin/${COMPILER_TYPE}nm)

# Pure magic according following http://stackoverflow.com/questions/41589430/cmake-c-compiler-identification-fails
# to get rid of the many try_compile attempts by CMake
SET(CMAKE_C_COMPILER_WORKS TRUE CACHE INTERNAL "")
SET(CMAKE_CXX_COMPILER_WORKS TRUE CACHE INTERNAL "")

SET(CMAKE_C_COMPILER_FORCED TRUE CACHE INTERNAL "")
SET(CMAKE_CXX_COMPILER_FORCED TRUE CACHE INTERNAL "")

SET(CMAKE_C_COMPILER_ID_RUN TRUE CACHE INTERNAL "")
SET(CMAKE_CXX_COMPILER_ID_RUN TRUE CACHE INTERNAL "")

SET(DEFAULT_CXX_FLAGS       "-std=c++11 -fno-exceptions -fdelete-dead-exceptions -fno-unwind-tables -fno-non-call-exceptions")
SET(DEFAULT_C_FLAGS         "-std=gnu99")
SET(DEFAULT_C_AND_CXX_FLAGS "-mthumb -ffunction-sections -fdata-sections -g3 -Wall -Werror")

# Collect flags that have to do with optimization
# We are optimizing for SIZE for now. If size turns out to be abundant, enable -O3 optimization.
# Interesting options: -ffast-math, -flto (link time optimization)
# Regarding size of the binary -ffast-math nor -flto makes a difference
SET(DEFAULT_C_AND_CXX_FLAGS "${DEFAULT_C_AND_CXX_FLAGS} -Os -fomit-frame-pointer")

# There is a bug in CMAKE_OBJCOPY, it doesn't exist on execution for the first time
SET(CMAKE_OBJCOPY_OVERLOAD                       ${COMPILER_PATH}/bin/${COMPILER_TYPE}objcopy)

# The following require FORCE. Without it, the FLAGS end up to have duplication.
SET(CMAKE_CXX_FLAGS                              ${DEFAULT_CXX_FLAGS}          CACHE STRING "C++ flags" FORCE)
SET(CMAKE_C_FLAGS                                ${DEFAULT_C_FLAGS}            CACHE STRING "C flags" FORCE)
SET(CMAKE_C_AND_CXX_FLAGS                        ${DEFAULT_C_AND_CXX_FLAGS}    CACHE STRING "C and C++ flags" FORCE)
SET(CMAKE_SHARED_LINKER_FLAGS                    ""                            CACHE STRING "Shared linker flags" FORCE)
SET(CMAKE_MODULE_LINKER_FLAGS                    ""                            CACHE STRING "Module linker flags" FORCE)
SET(CMAKE_EXE_LINKER_FLAGS                       "-Wl,-z,nocopyreloc"          CACHE STRING "Executable linker flags" FORCE)
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS            "")
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS          "")

# The directory with some of the FindXXX modules
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/conf;${CMAKE_SOURCE_DIR}/conf/cmake")

MESSAGE(STATUS "ARM cmake: C Compiler: ${CMAKE_C_COMPILER}")
MESSAGE(STATUS "ARM cmake: Search for FindX files in ${CMAKE_MODULE_PATH}")

INCLUDE(crownstone.defs)

# the bluetooth name is not optional
IF(DEFINED BLUETOOTH_NAME)
	ADD_DEFINITIONS("-DBLUETOOTH_NAME=${BLUETOOTH_NAME}")
ELSE()
	MESSAGE(FATAL_ERROR "We require a BLUETOOTH_NAME in CMakeBuild.config (5 characters or less), i.e. \"Crown\" (with quotes)")
ENDIF()

# Obtain variables to be used for the compiler
SET(NRF51822_DIR                                "${NRF51822_DIR}"                   CACHE STRING "Nordic SDK Directory" FORCE)
SET(NORDIC_SDK_VERSION                          "${NORDIC_SDK_VERSION}"             CACHE STRING "Nordic SDK Version" FORCE)
SET(SOFTDEVICE_SERIES                           "${SOFTDEVICE_SERIES}"              CACHE STRING "SOFTDEVICE_SERIES" FORCE)
SET(SOFTDEVICE_MAJOR                            "${SOFTDEVICE_MAJOR}"               CACHE STRING "SOFTDEVICE_MAJOR" FORCE)
SET(SOFTDEVICE_MINOR                            "${SOFTDEVICE_MINOR}"               CACHE STRING "SOFTDEVICE_MINOR" FORCE)
SET(SOFTDEVICE                                  "${SOFTDEVICE}"                     CACHE STRING "SOFTDEVICE" FORCE)
SET(SOFTDEVICE_NO_SEPARATE_UICR_SECTION         "${SOFTDEVICE_NO_SEPARATE_UICR_SECTION}"   CACHE STRING "SOFTDEVICE_NO_SEPARATE_UICR_SECTION" FORCE)
SET(APPLICATION_START_ADDRESS                   "${APPLICATION_START_ADDRESS}"      CACHE STRING "APPLICATION_START_ADDRESS" FORCE)
SET(APPLICATION_LENGTH                          "${APPLICATION_LENGTH}"             CACHE STRING "APPLICATION_LENGTH" FORCE)
SET(COMPILATION_TIME                            "${COMPILATION_TIME}"               CACHE STRING "COMPILATION_TIME" FORCE)
SET(COMPILATION_DAY                             "${COMPILATION_DAY}"                CACHE STRING "COMPILATION_DAY" FORCE)
IF(DEFINED GIT_HASH)
	SET(GIT_HASH                                "${GIT_HASH}"                       CACHE STRING "GIT_HASH" FORCE)
ENDIF()
#SET(HARDWARE_BOARD                              "${HARDWARE_BOARD}"                 CACHE STRING "HARDWARE_BOARD" FORCE)
SET(SERIAL_VERBOSITY                            "${SERIAL_VERBOSITY}"               CACHE STRING "SERIAL_VERBOSITY" FORCE)
SET(MASTER_BUFFER_SIZE                          "${MASTER_BUFFER_SIZE}"             CACHE STRING "MASTER_BUFFER_SIZE" FORCE)
SET(DEFAULT_ON                                  "${DEFAULT_ON}"                     CACHE STRING "DEFAULT_ON" FORCE)
SET(RSSI_ENABLE                                 "${RSSI_ENABLE}"                    CACHE STRING "RSSI_ENABLE" FORCE)
SET(TX_POWER                                    "${TX_POWER}"                       CACHE STRING "TX_POWER" FORCE)
SET(ADVERTISEMENT_INTERVAL                      "${ADVERTISEMENT_INTERVAL}"         CACHE STRING "ADVERTISEMENT_INTERVAL" FORCE)
SET(MIN_ENV_TEMP                                "${MIN_ENV_TEMP}"                   CACHE STRING "MIN_ENV_TEMP" FORCE)
SET(MAX_ENV_TEMP                                "${MAX_ENV_TEMP}"                   CACHE STRING "MAX_ENV_TEMP" FORCE)
SET(LOW_POWER_MODE                              "${LOW_POWER_MODE}"                 CACHE STRING "LOW_POWER_MODE" FORCE)
SET(PWM_ENABLE                                  "${PWM_ENABLE}"                     CACHE STRING "PWM_ENABLE" FORCE)
SET(FIRMWARE_VERSION                            "${FIRMWARE_VERSION}"               CACHE STRING "FIRMWARE_VERSION" FORCE)
SET(BOOT_DELAY                                  "${BOOT_DELAY}"                     CACHE STRING "BOOT_DELAY" FORCE)
SET(SCAN_DURATION                               "${SCAN_DURATION}"                  CACHE STRING "SCAN_DURATION" FORCE)
SET(SCAN_SEND_DELAY                             "${SCAN_SEND_DELAY}"                CACHE STRING "SCAN_SEND_DELAY" FORCE)
SET(SCAN_BREAK_DURATION                         "${SCAN_BREAK_DURATION}"            CACHE STRING "SCAN_BREAK_DURATION" FORCE)
SET(MAX_CHIP_TEMP                               "${MAX_CHIP_TEMP}"                  CACHE STRING "MAX_CHIP_TEMP" FORCE)
SET(SCAN_FILTER                                 "${SCAN_FILTER}"                    CACHE STRING "SCAN_FILTER" FORCE)
SET(SCAN_FILTER_SEND_FRACTION                   "${SCAN_FILTER_SEND_FRACTION}"      CACHE STRING "SCAN_FILTER_SEND_FRACTION" FORCE)
SET(INTERVAL_SCANNER_ENABLED                    "${INTERVAL_SCANNER_ENABLED}"       CACHE STRING "INTERVAL_SCANNER_ENABLED" FORCE)
SET(CONTINUOUS_POWER_SAMPLER                    "${CONTINUOUS_POWER_SAMPLER}"       CACHE STRING "CONTINUOUS_POWER_SAMPLER" FORCE)
SET(DEFAULT_OPERATION_MODE                      "${DEFAULT_OPERATION_MODE}"         CACHE STRING "DEFAULT_OPERATION_MODE" FORCE)
SET(PERSISTENT_FLAGS_DISABLED                   "${PERSISTENT_FLAGS_DISABLED}"      CACHE STRING "PERSISTENT_FLAGS_DISABLED" FORCE)
SET(MESH_NUM_HANDLES                            "${MESH_NUM_HANDLES}"               CACHE STRING "MESH_NUM_HANDLES" FORCE)
SET(MESH_ACCESS_ADDR                            "${MESH_ACCESS_ADDR}"               CACHE STRING "MESH_ACCESS_ADDR" FORCE)
SET(MESH_INTERVAL_MIN_MS                        "${MESH_INTERVAL_MIN_MS}"           CACHE STRING "MESH_INTERVAL_MIN_MS" FORCE)
SET(MESH_CHANNEL                                "${MESH_CHANNEL}"                   CACHE STRING "MESH_CHANNEL" FORCE)
SET(MESH_BOOT_TIME                              "${MESH_BOOT_TIME}"                 CACHE STRING "MESH_BOOT_TIME" FORCE)
SET(NRF_SERIES                                  "${NRF_SERIES}"                     CACHE STRING "NRF_SERIES" FORCE)
SET(RAM_R1_BASE                                 "${RAM_R1_BASE}"                    CACHE STRING "RAM_R1_BASE" FORCE)
SET(MAX_NUM_VS_SERVICES                         "${MAX_NUM_VS_SERVICES}"            CACHE STRING "MAX_NUM_VS_SERVICES" FORCE)
SET(ADVERTISEMENT_IMPROVEMENT                   "${ADVERTISEMENT_IMPROVEMENT}"      CACHE STRING "ADVERTISEMENT_IMPROVEMENT" FORCE)
IF(DEFINED RELAY_DEFAULT_ON)
SET(RELAY_DEFAULT_ON                            "${RELAY_DEFAULT_ON}"               CACHE STRING "RELAY_DEFAULT_ON" FORCE)
ENDIF()
SET(CONNECTION_ALIVE_TIMEOUT                    "${CONNECTION_ALIVE_TIMEOUT}"       CACHE STRING "CONNECTION_ALIVE_TIMEOUT" FORCE)

# Set Attribute table size
SET(ATTR_TABLE_SIZE                             "${ATTR_TABLE_SIZE}"                CACHE STRING "ATTR_TABLE_SIZE" FORCE)

# Add encryption
SET(ENCRYPTION                                  "${ENCRYPTION}"                     CACHE STRING "ENCRYPTION" FORCE)
SET(STATIC_PASSKEY                              "${STATIC_PASSKEY}"                 CACHE STRING "STATIC_PASSKEY" FORCE)

# SET(DEVICE_TYPE                                 "${DEVICE_TYPE}"                    CACHE STRING "DEVICE_TYPE" FORCE)

SET(MESHING                                     "${MESHING}"                        CACHE STRING "MESHING" FORCE)
SET(BUILD_MESHING                               "${BUILD_MESHING}"                  CACHE STRING "BUILD_MESHING" FORCE)

# Add iBeacon default values
SET(IBEACON                                     "${IBEACON}"                        CACHE STRING "IBEACON" FORCE)

SET(BEACON_UUID                                 "${BEACON_UUID}"                    CACHE STRING "BEACON_UUID" FORCE)
SET(BEACON_MAJOR                                "${BEACON_MAJOR}"                   CACHE STRING "BEACON_MAJOR" FORCE)
SET(BEACON_MINOR                                "${BEACON_MINOR}"                   CACHE STRING "BEACON_MINOR" FORCE)
SET(BEACON_RSSI                                 "${BEACON_RSSI}"                    CACHE STRING "BEACON_RSSI" FORCE)

SET(EDDYSTONE                                   "${EDDYSTONE}"                      CACHE STRING "EDDYSTONE" FORCE)

# Add services
SET(CROWNSTONE_SERVICE                          "${CROWNSTONE_SERVICE}"             CACHE STRING "CROWNSTONE_SERVICE" FORCE)
SET(INDOOR_SERVICE                              "${INDOOR_SERVICE}"                 CACHE STRING "INDOOR_SERVICE" FORCE)
SET(GENERAL_SERVICE                             "${GENERAL_SERVICE}"                CACHE STRING "GENERAL_SERVICE" FORCE)
SET(POWER_SERVICE                               "${POWER_SERVICE}"                  CACHE STRING "POWER_SERVICE" FORCE)
SET(SCHEDULE_SERVICE                            "${SCHEDULE_SERVICE}"               CACHE STRING "SCHEDULE_SERVICE" FORCE)

# Add characteristics
SET(CHAR_CONTROL                                "${CHAR_CONTROL}"                   CACHE STRING "CHAR_CONTROL" FORCE)
SET(CHAR_MESHING                                "${CHAR_MESHING}"                   CACHE STRING "CHAR_MESHING" FORCE)
SET(CHAR_TEMPERATURE                            "${CHAR_TEMPERATURE}"               CACHE STRING "CHAR_TEMPERATURE" FORCE)
SET(CHAR_RESET                                  "${CHAR_RESET}"                     CACHE STRING "CHAR_RESET" FORCE)
SET(CHAR_CONFIGURATION                          "${CHAR_CONFIGURATION}"             CACHE STRING "CHAR_CONFIGURATION" FORCE)
SET(CHAR_STATE                                  "${CHAR_STATE}"                     CACHE STRING "CHAR_STATE" FORCE)
SET(CHAR_PWM                                    "${CHAR_PWM}"                       CACHE STRING "CHAR_PWM" FORCE)
SET(CHAR_SAMPLE_CURRENT                         "${CHAR_SAMPLE_CURRENT}"            CACHE STRING "CHAR_SAMPLE_CURRENT" FORCE)
SET(CHAR_CURRENT_LIMIT                          "${CHAR_CURRENT_LIMIT}"             CACHE STRING "CHAR_CURRENT_LIMIT" FORCE)
SET(CHAR_RSSI                                   "${CHAR_RSSI}"                      CACHE STRING "CHAR_RSSI" FORCE)
SET(CHAR_SCAN_DEVICES                           "${CHAR_SCAN_DEVICES}"              CACHE STRING "CHAR_SCAN_DEVICES" FORCE)
SET(CHAR_TRACK_DEVICES                          "${CHAR_TRACK_DEVICES}"             CACHE STRING "CHAR_TRACK_DEVICES" FORCE)
SET(CHAR_RELAY                                  "${CHAR_RELAY}"                     CACHE STRING "CHAR_RELAY" FORCE)
SET(CHAR_SCHEDULE                               "${CHAR_SCHEDULE}"                  CACHE STRING "CHAR_SCHEDULE" FORCE)

GET_DIRECTORY_PROPERTY( DirDefs DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS )

FOREACH(definition ${DirDefs})
	IF(VERBOSITY GREATER 4)
		MESSAGE(STATUS "ARM cmake: Definition: " ${definition})
	ENDIF()
	IF(${definition} MATCHES "=$")
		IF(NOT ${definition} MATCHES "COMPILATION_TIME")
			MESSAGE(STATUS "ARM cmake: Definition ${definition} is not defined" )
		ENDIF()
	ENDIF()
ENDFOREACH()

# Set compiler flags

# Only from nRF52 onwards we have a coprocessor for floating point operations
IF(NRF_SERIES MATCHES NRF52)
	SET(CMAKE_C_AND_CXX_FLAGS "${CMAKE_C_AND_CXX_FLAGS} -mcpu=cortex-m4")
	SET(CMAKE_C_AND_CXX_FLAGS "${CMAKE_C_AND_CXX_FLAGS} -mfloat-abi=hard -mfpu=fpv4-sp-d16")
ENDIF()

IF(PRINT_FLOATS)
	SET(CMAKE_C_AND_CXX_FLAGS "${CMAKE_C_AND_CXX_FLAGS} -u _printf_float")
ENDIF()

# Final collection of C/C++ compiler flags

MESSAGE(STATUS "ARM cmake: CXX flags: ${CMAKE_CXX_FLAGS}")
MESSAGE(STATUS "ARM cmake: C flags: ${CMAKE_C_FLAGS}")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_AND_CXX_FLAGS} ${DEFINES}")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_AND_CXX_FLAGS} ${DEFINES}")

# Tell the linker that we use a special memory layout
SET(FILE_MEMORY_LAYOUT "-TnRF51822-softdevice.ld")
SET(PATH_FILE_MEMORY "-L${PROJECT_SOURCE_DIR}/conf")

# http://public.kitware.com/Bug/view.php?id=12652
# CMake does send the compiler flags also to the linker

SET(FLAG_WRITE_MAP_FILE "-Wl,-Map,prog.map")
#SET(FLAG_REMOVE_UNWINDING_CODE "-Wl,--wrap,__aeabi_unwind_cpp_pr0")
SET(FLAG_REMOVE_UNWINDING_CODE "")

# do not define above as multiple linker flags, or else you will get redefines of MEMORY etc.
SET(CMAKE_EXE_LINKER_FLAGS "${PATH_FILE_MEMORY} ${FILE_MEMORY_LAYOUT} -Wl,--gc-sections ${CMAKE_EXE_LINKER_FLAGS} ${FLOAT_FLAGS} ${FLAG_WRITE_MAP_FILE} ${FLAG_REMOVE_UNWINDING_CODE}")

# We preferably want to run the cross-compiler tests without all the flags. This namely means we have to add for example the object out of syscalls.c to the compilation, etc. Or, differently, have different flags for the compiler tests. This is difficult to do!
#SET(CMAKE_C_FLAGS "-nostdlib")

# find the libraries
# http://qmcpack.cmscc.org/getting-started/using-cmake-toolchain-file

# set the installation root (should contain usr/local and usr/lib directories)
SET(DESTDIR /data/arm)

# here will the header files be installed on "make install"
SET(CMAKE_INSTALL_PREFIX ${DESTDIR}/usr/local)

# add the libraries from the installation directory (if they have been build before)
#LINK_DIRECTORIES("${DESTDIR}/usr/local/lib")
#LINK_DIRECTORIES("${COMPILER_PATH}/${COMPILER_TYPE}/lib")

# the following doesn't seem to work so well
SET(CMAKE_INCLUDE_PATH ${DESTDIR}/usr/local/include)
MESSAGE(STATUS "ARM cmake: Add include path: ${CMAKE_INCLUDE_PATH}")

# indicate where the linker is allowed to search for library / headers
#SET(CMAKE_FIND_ROOT_PATH
	#${ARM_LINUX_SYSROOT}
	#	${DESTDIR})
#SET(CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH} ${ARM_LINUX_SYSROOT})

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

