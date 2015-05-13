#######################################################################################################################
# The build systems uses CMake. All the automatically generated code falls under the Lesser General Public License
# (LGPL GNU v3), the Apache License, or the MIT license, your choice.
#
# Author:	 Anne C. van Rossum (Distributed Organisms B.V.)
# Date: 	 Oct 28, 2013
#
# Copyright © 2013 Anne C. van Rossum <anne@dobots.nl>
#######################################################################################################################
#Check http://www.cmake.org/Wiki/CMake_Cross_Compiling

# Set to Generic, or tests will fail (they will fail anyway)
SET(CMAKE_SYSTEM_NAME Generic)

# Tell we want to cross-compile
SET(CMAKE_SYSTEM_VERSION 1)
SET(CMAKE_SYSTEM_PROCESSOR arm)
SET(CMAKE_CROSSCOMPILING 1)

# Load compiler options from configuration file
SET(CONFIG_FILE ${CMAKE_SOURCE_DIR}/CMakeConfig.cmake)
MESSAGE(STATUS "Load config file ${CONFIG_FILE}")
INCLUDE(${CONFIG_FILE})

UNSET(CMAKE_TRY_COMPILE_CONFIGURATION)

# type of compiler we want to use
SET(COMPILER_TYPE_PREFIX ${COMPILER_TYPE}-)

# The extension .obj is just ugly, set it back to .o (does not work)
SET(CMAKE_CXX_OUTPUT_EXTENSION .o)

# Make cross-compiler easy to find (but we will use absolute paths anyway)
SET(PATH "${PATH}:${COMPILER_PATH}/bin")
MESSAGE(STATUS "PATH is set to: ${PATH}")

#SET(ARM_LINUX_SYSROOT /opt/compiler/gcc-arm-none-eabi-${COMPILER_VERSION} CACHE PATH "ARM cross compilation system root")

# Specify the cross compiler, linker, etc.
SET(CMAKE_C_COMPILER	${COMPILER_PATH}/bin/${COMPILER_TYPE}-gcc)
SET(CMAKE_CXX_COMPILER	${COMPILER_PATH}/bin/${COMPILER_TYPE}-g++)
SET(CMAKE_ASM		${COMPILER_PATH}/bin/${COMPILER_TYPE}-as)
SET(CMAKE_LINKER	${COMPILER_PATH}/bin/${COMPILER_TYPE}-ld)
SET(CMAKE_READELF	${COMPILER_PATH}/bin/${COMPILER_TYPE}-readelf)
SET(CMAKE_OBJDUMP	${COMPILER_PATH}/bin/${COMPILER_TYPE}-objdump)
SET(CMAKE_OBJCOPY	${COMPILER_PATH}/bin/${COMPILER_TYPE}-objcopy)
SET(CMAKE_SIZE		${COMPILER_PATH}/bin/${COMPILER_TYPE}-size)
SET(CMAKE_NM		${COMPILER_PATH}/bin/${COMPILER_TYPE}-nm)

# There is a bug in CMAKE_OBJCOPY, it doesn't exist on execution for the first time
SET(CMAKE_OBJCOPY_OVERLOAD                       ${COMPILER_PATH}/bin/${COMPILER_TYPE}-objcopy)

SET(CMAKE_CXX_FLAGS                              "-std=c++11 -fno-exceptions -fdelete-dead-exceptions -fno-unwind-tables -fno-non-call-exceptions"                    CACHE STRING "c++ flags")
SET(CMAKE_C_FLAGS                                "-std=gnu99"                                    CACHE STRING "c flags")
SET(CMAKE_SHARED_LINKER_FLAGS                    ""                                              CACHE STRING "shared linker flags")
SET(CMAKE_MODULE_LINKER_FLAGS                    ""                                              CACHE STRING "module linker flags")
SET(CMAKE_EXE_LINKER_FLAGS                       "-Wl,-z,nocopyreloc"                            CACHE STRING "executable linker flags")
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS            "")
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS          "")

# Collect flags that have to do with optimization, optimize for size for now
#SET(FLAG_MATH "-ffast-math")
#SET(FLAG_LINK_TIME_OPTIMIZATION "-flto")
SET(OPTIMIZE "-Os -fomit-frame-pointer ${FLAG_MATH} ${FLAG_LINK_TIME_OPTIMIZATION}")
#SET(OPTIMIZE "-O3 -fomit-frame-pointer ${FLAG_MATH} ${FLAG_LINK_TIME_OPTIMIZATION}")

# Collect flags that are used in the code, as macros
#ADD_DEFINITIONS("-MMD -DNRF51 -DEPD_ENABLE_EXTRA_RAM -DARDUINO=100 -DE_STICKY_v1 -DNRF51_USE_SOFTDEVICE=${NRF51_USE_SOFTDEVICE} -DUSE_RENDER_CONTEXT -DSYSCALLS -DUSING_FUNC")
ADD_DEFINITIONS("-MMD -DNRF51 -DEPD_ENABLE_EXTRA_RAM -DNRF51_USE_SOFTDEVICE=${NRF51_USE_SOFTDEVICE} -DUSE_RENDER_CONTEXT -DSYSCALLS -DUSING_FUNC")

LIST(APPEND CUSTOM_DEFINITIONS, TEMPERATURE)

ADD_DEFINITIONS("-DBLE_STACK_SUPPORT_REQD")

# Pass variables in defined in the configuration file to the compiler
ADD_DEFINITIONS("-DNRF51822_DIR=${NRF51822_DIR}")
ADD_DEFINITIONS("-DNORDIC_SDK_VERSION=${NORDIC_SDK_VERSION}")
ADD_DEFINITIONS("-DSOFTDEVICE_SERIES=${SOFTDEVICE_SERIES}")
ADD_DEFINITIONS("-DSOFTDEVICE_MAJOR=${SOFTDEVICE_MAJOR}")
ADD_DEFINITIONS("-DSOFTDEVICE_MINOR=${SOFTDEVICE_MINOR}")
ADD_DEFINITIONS("-DSOFTDEVICE=${SOFTDEVICE}")
ADD_DEFINITIONS("-DSOFTDEVICE_NO_SEPARATE_UICR_SECTION=${SOFTDEVICE_NO_SEPARATE_UICR_SECTION}")
ADD_DEFINITIONS("-DAPPLICATION_START_ADDRESS=${APPLICATION_START_ADDRESS}")
ADD_DEFINITIONS("-DAPPLICATION_LENGTH=${APPLICATION_LENGTH}")
ADD_DEFINITIONS("-DCOMPILATION_TIME=${COMPILATION_TIME}")
ADD_DEFINITIONS("-DBOARD=${BOARD}")
ADD_DEFINITIONS("-DHARDWARE_VERSION=${HARDWARE_VERSION}")
ADD_DEFINITIONS("-DSERIAL_VERBOSITY=${SERIAL_VERBOSITY}")
ADD_DEFINITIONS("-DMASTER_BUFFER_SIZE=${MASTER_BUFFER_SIZE}")
ADD_DEFINITIONS("-DDEFAULT_ON=${DEFAULT_ON}")

ADD_DEFINITIONS("-DIBEACON=${IBEACON}")
IF(IBEACON)
ADD_DEFINITIONS("-DBEACON_UUID=${BEACON_UUID}")
ADD_DEFINITIONS("-DBEACON_MAJOR=${BEACON_MAJOR}")
ADD_DEFINITIONS("-DBEACON_MINOR=${BEACON_MINOR}")
ADD_DEFINITIONS("-DBEACON_RSSI=${BEACON_RSSI}")
ENDIF()

# Add services
ADD_DEFINITIONS("-DINDOOR_SERVICE=${INDOOR_SERVICE}")
ADD_DEFINITIONS("-DGENERAL_SERVICE=${GENERAL_SERVICE}")
ADD_DEFINITIONS("-DPOWER_SERVICE=${POWER_SERVICE}")

# Add characteristics
ADD_DEFINITIONS("-DCHAR_MESHING=${CHAR_MESHING}")
ADD_DEFINITIONS("-DCHAR_TEMPERATURE=${CHAR_TEMPERATURE}")
ADD_DEFINITIONS("-DCHAR_RESET=${CHAR_RESET}")
ADD_DEFINITIONS("-DCHAR_CONFIGURATION=${CHAR_CONFIGURATION}")
ADD_DEFINITIONS("-DCHAR_PWM=${CHAR_PWM}")
ADD_DEFINITIONS("-DCHAR_SAMPLE_CURRENT=${CHAR_SAMPLE_CURRENT}")
ADD_DEFINITIONS("-DCHAR_CURRENT_LIMIT=${CHAR_CURRENT_LIMIT}")
ADD_DEFINITIONS("-DCHAR_RSSI=${CHAR_RSSI}")
ADD_DEFINITIONS("-DCHAR_SCAN_DEVICES=${CHAR_SCAN_DEVICES}")
ADD_DEFINITIONS("-DCHAR_TRACK_DEVICES=${CHAR_TRACK_DEVICES}")

# only required if Nordic files are used
ADD_DEFINITIONS("-DBOARD_NRF6310")

# the bluetooth name is optional
IF(BLUETOOTH_NAME)
	ADD_DEFINITIONS("-DBLUETOOTH_NAME=${BLUETOOTH_NAME}")
ELSE()
	MESSAGE(FATAL_ERROR "We require a BLUETOOTH_NAME in CMakeBuild.config (5 characters or less), i.e. \"Crown\" (with quotes)")
ENDIF()

GET_DIRECTORY_PROPERTY( DirDefs DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS )

FOREACH(definition ${DirDefs})
	MESSAGE(STATUS "Definition: " ${definition})
	IF(${definition} MATCHES "=$")
		IF(NOT ${definition} MATCHES "COMPILATION_TIME")
			MESSAGE(FATAL_ERROR "Definition ${definition} is not defined" )
		ENDIF()
	ENDIF()
ENDFOREACH()

# Set the compiler flags
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -Wall ${OPTIMIZE} -mcpu=cortex-m0 -mthumb -ffunction-sections -fdata-sections ${DEFINES}")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -Wall ${OPTIMIZE} -mcpu=cortex-m0 -mthumb -ffunction-sections -fdata-sections ${DEFINES}")

# Tell the linker that we use a special memory layout
SET(FILE_MEMORY_LAYOUT "-TnRF51822-softdevice.ld")
SET(PATH_FILE_MEMORY "-L${PROJECT_SOURCE_DIR}/conf")

# http://public.kitware.com/Bug/view.php?id=12652
# CMake does send the compiler flags also to the linker

SET(FLAG_WRITE_MAP_FILE "-Wl,-Map,prog.map")
SET(FLAG_REMOVE_UNWINDING_CODE "-Wl,--wrap,__aeabi_unwind_cpp_pr0")

# do not define above as multiple linker flags, or else you will get redefines of MEMORY etc.
SET(CMAKE_EXE_LINKER_FLAGS "${PATH_FILE_MEMORY} ${FILE_MEMORY_LAYOUT} -Wl,--gc-sections ${CMAKE_EXE_LINKER_FLAGS} ${FLAG_WRITE_MAP_FILE} ${FLAG_REMOVE_UNWINDING_CODE}")

# We preferably want to run the cross-compiler tests without all the flags. This namely means we have to add for example the object out of syscalls.c to the compilation, etc. Or, differently, have different flags for the compiler tests. This is difficult to do!
#SET(CMAKE_C_FLAGS "-nostdlib")

#SET(CMAKE_C_COMPILER_WORKS 1)
#SET(CMAKE_CXX_COMPILER_WORKS 1)

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
MESSAGE(STATUS "Add include path: ${CMAKE_INCLUDE_PATH}")

# indicate where the linker is allowed to search for library / headers
SET(CMAKE_FIND_ROOT_PATH
	#${ARM_LINUX_SYSROOT}
	${DESTDIR})
#SET(CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH} ${ARM_LINUX_SYSROOT})

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

