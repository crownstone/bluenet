######################################################################################################################
# The build systems uses CMake. All the automatically generated code falls under the Lesser General Public License
# (LGPL GNU v3), the Apache License, or the MIT license, your choice.
#
# Author:	 Crownstone Team
# Date: 	 Oct 28, 2013
#
# Copyright © 2013 Crownstone B.V. <team@crownstone.rocks>
#######################################################################################################################
#
# Check http://www.cmake.org/Wiki/CMake_Cross_Compiling
#
# Compiler options are already set through the main CMakeLists.txt file that loads this as an ExternalProject.

# Version to be used
CMAKE_MINIMUM_REQUIRED(VERSION 3.9)

# Set to Generic, or tests will fail (they will fail anyway)
SET(CMAKE_SYSTEM_NAME Generic)

# Tell we want to cross-compile
SET(CMAKE_SYSTEM_VERSION 1)
SET(CMAKE_SYSTEM_PROCESSOR arm)
SET(CMAKE_CROSSCOMPILING 1)

MESSAGE(STATUS "Bluetooth name ${BLUETOOTH_NAME}")

INCLUDE(CheckIPOSupported)

# Set a particular build type (do not add to cache)
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE                         Debug)
endif()

#######################################################################################################################

# type of compiler we want to use, the COMPILER_TYPE can be empty if normal gcc and g++ compilers are intended
SET(COMPILER_TYPE_PREFIX ${COMPILER_TYPE})

#######################################################################################################################

# Make cross-compiler easy to find (but we will use absolute paths anyway)
SET(PATH "${PATH}:${COMPILER_PATH}/bin")
MESSAGE(STATUS "PATH is set to: ${PATH}")

# Actually add to PATH
LIST(APPEND CMAKE_PROGRAM_PATH ${COMPILER_PATH}/bin)

# Specify the cross compiler, linker, etc.
SET(CMAKE_C_COMPILER                   ${COMPILER_PATH}/bin/${COMPILER_TYPE}gcc)
SET(CMAKE_CXX_COMPILER                 ${COMPILER_PATH}/bin/${COMPILER_TYPE}g++)
SET(CMAKE_ASM_COMPILER                 ${COMPILER_PATH}/bin/${COMPILER_TYPE}as)
SET(CMAKE_LINKER                       ${COMPILER_PATH}/bin/${COMPILER_TYPE}ld)
SET(CMAKE_READELF                      ${COMPILER_PATH}/bin/${COMPILER_TYPE}readelf)
SET(CMAKE_OBJDUMP                      ${COMPILER_PATH}/bin/${COMPILER_TYPE}objdump)
SET(CMAKE_OBJCOPY                      ${COMPILER_PATH}/bin/${COMPILER_TYPE}objcopy)
SET(CMAKE_SIZE                         ${COMPILER_PATH}/bin/${COMPILER_TYPE}size)
SET(CMAKE_NM                           ${COMPILER_PATH}/bin/${COMPILER_TYPE}nm)

#ENABLE_LANGUAGE(ASM)

# Pure magic according following http://stackoverflow.com/questions/41589430/cmake-c-compiler-identification-fails
# to get rid of the many try_compile attempts by CMake
SET(CMAKE_C_COMPILER_WORKS TRUE CACHE INTERNAL "")
SET(CMAKE_CXX_COMPILER_WORKS TRUE CACHE INTERNAL "")

SET(CMAKE_C_COMPILER_FORCED TRUE CACHE INTERNAL "")
SET(CMAKE_CXX_COMPILER_FORCED TRUE CACHE INTERNAL "")

SET(CMAKE_C_COMPILER_ID_RUN TRUE CACHE INTERNAL "")
SET(CMAKE_CXX_COMPILER_ID_RUN TRUE CACHE INTERNAL "")

SET(DEBUG_FLAGS   "-g3")
SET(RELEASE_FLAGS "-g0")

# The default flags will be used in all build types
# The C/C++ shared flags are in DEFAULT_C_AND_CXX_FLAGS, the ones particular for C or C++ in DEFAULT_X_FLAGS
# The flags mean the following:
#   * no-exceptions          disable exceptions support and use C++ libs without exceptions
#   * delete-dead-exceptions throw away instructions which only purpose is to throw exceptions
#   * no-unwind-tables       stack-walking code for backtrace generation (TODO: we might want to enable this in Debug!)
#   * no-non-call-exceptions do not convert trapping instructions (SIGSEGV, etc) into exceptions 
#   * thumb                  use Thumb mode (optimized instruction set)
#   * function-sections      put every function in its own segment, now it can be garbage collected
#   * data-sections          put every variable in its own segment
#   * no-strict-aliasing     allow *int = *float aberrations
#   * no-builtin             do not use abs, strcpy, and other built-in functions
#   * short-enums            use a single byte for an enum if possible
# 
SET(DEFAULT_CXX_FLAGS       "-std=c++17 -fno-exceptions -fdelete-dead-exceptions -fno-unwind-tables -fno-non-call-exceptions")
SET(DEFAULT_C_FLAGS         "")
SET(DEFAULT_C_AND_CXX_FLAGS "-mthumb -ffunction-sections -fdata-sections -Wall -Werror -fdiagnostics-color=always -fno-strict-aliasing -fno-builtin -fshort-enums -Wno-error=format -Wno-error=unused-function")

SET(ASM_OPTIONS "-x assembler-with-cpp")
SET(CMAKE_ASM_FLAGS "${CFLAGS} ${ASM_OPTIONS}" )

SET(DEFAULT_C_AND_CXX_FLAGS "${DEFAULT_C_AND_CXX_FLAGS} -Os -fomit-frame-pointer")

# The option --print-gc-section can be used to debug which sections are actually garbage collected
SET(GARBAGE_COLLECTION_OF_SECTIONS "-Wl,--gc-sections")

# Use of newlib-nano
SET(OPTIMIZED_NEWLIB "--specs=nano.specs")

# There is a bug in CMAKE_OBJCOPY, it doesn't exist on execution for the first time
SET(CMAKE_OBJCOPY_OVERLOAD                       ${COMPILER_PATH}/bin/${COMPILER_TYPE}objcopy)

# The following require FORCE. Without it, the FLAGS end up to have duplication.
SET(CMAKE_CXX_FLAGS                              "${DEFAULT_CXX_FLAGS}"        CACHE STRING "C++ flags" FORCE)
SET(CMAKE_C_FLAGS                                "${DEFAULT_C_FLAGS}"          CACHE STRING "C flags" FORCE)
SET(CMAKE_SHARED_LINKER_FLAGS                    ""                            CACHE STRING "Shared linker flags" FORCE)
SET(CMAKE_MODULE_LINKER_FLAGS                    ""                            CACHE STRING "Module linker flags" FORCE)
SET(CMAKE_EXE_LINKER_FLAGS                       "-Wl,-z,nocopyreloc ${GARBAGE_COLLECTION_OF_SECTIONS} ${OPTIMIZED_NEWLIB}"          CACHE STRING "Executable linker flags" FORCE)
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS            "")
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS          "")

# The directory with some of the FindXXX modules
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/conf;${CMAKE_SOURCE_DIR}/conf/cmake;${DEFAULT_MODULES_PATH};${DEFAULT_CONF_CMAKE_PATH}")

MESSAGE(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
MESSAGE(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
MESSAGE(STATUS "Search for FindX files in ${CMAKE_MODULE_PATH}")

INCLUDE(crownstone.defs)
INCLUDE(colors)

# Collect flags that have to do with optimization
# We are optimizing for SIZE for now. If size turns out to be abundant, enable -O3 optimization.
# Interesting options: -ffast-math, -flto (link time optimization), -fno-rtti
# Regarding size of the binary -ffast-math made a very small difference (4B), while -flto made a difference of almost 13kB.
# When adding -flto to C_FLAGS, you also need -fno-builtin, else it gives an error of multiple definitions of _exit().
#SET(DEFAULT_C_AND_CXX_FLAGS "${DEFAULT_C_AND_CXX_FLAGS} -flto -fno-builtin")
CHECK_IPO_SUPPORTED(RESULT result)
IF(result)
	MESSAGE(STATUS "Enabled interprocedural optimization: -lto")
	SET(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
	MESSAGE(STATUS "${Red}Note! Interprocedural optimization not enabled (might just be because cmake does not know if the compiler supports it)${ColourReset}")
ENDIF()

IF(CMAKE_BUILD_TYPE MATCHES "Debug")
	MESSAGE(STATUS "${Red}Pay attention, this is a debug build (change with -DCMAKE_BUILD_TYPE=Release or RelWithDebInfo${ColourReset}")
ENDIF()

GET_DIRECTORY_PROPERTY( DirDefs DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS )

FOREACH(definition ${DirDefs})
	IF(VERBOSITY GREATER 4)
		MESSAGE(STATUS "Definition: " ${definition})
	ENDIF()
	IF(${definition} MATCHES "=$")
		IF(NOT ${definition} MATCHES "COMPILATION_TIME")
			MESSAGE(STATUS "Definition ${definition} is not defined" )
		ENDIF()
	ENDIF()
ENDFOREACH()

# Set compiler flags

# Only from nRF52 onwards we have a coprocessor for floating point operations
IF(NRF_DEVICE_FAMILY MATCHES NRF52)
	MESSAGE(STATUS "Enable hardware floating point support")
	SET(DEFAULT_C_AND_CXX_FLAGS "${DEFAULT_C_AND_CXX_FLAGS} -mcpu=cortex-m4")
	SET(DEFAULT_C_AND_CXX_FLAGS "${DEFAULT_C_AND_CXX_FLAGS} -mfloat-abi=hard -mfpu=fpv4-sp-d16")
	
	# The following is disabled due to size constraints, don't use %f in plain-text logging
	MESSAGE(STATUS "Plain-text log (printf) will not support floats due to size limitations")
	#SET(DEFAULT_C_AND_CXX_FLAGS "${DEFAULT_C_AND_CXX_FLAGS} -u _printf_float")
ELSE()
	MESSAGE(STATUS "Unknown hardware support for floating point ops")
ENDIF()

IF(PRINT_FLOATS)
	SET(DEFAULT_C_AND_CXX_FLAGS "${DEFAULT_C_AND_CXX_FLAGS} -u _printf_float")
ENDIF()

# Final collection of C/C++ compiler flags

MESSAGE(STATUS "CXX flags: ${CMAKE_CXX_FLAGS} ${DEFAULT_C_AND_CXX_FLAGS}")
MESSAGE(STATUS "C flags: ${CMAKE_C_FLAGS} ${DEFAULT_C_AND_CXX_FLAGS}")

# __STARTUP_CONFIG is defined in gcc_startup_nrf52.S

# Flags used in all configurations
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DEFAULT_C_AND_CXX_FLAGS} ${DEFINES}")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DEFAULT_C_AND_CXX_FLAGS} ${DEFINES}")

# Flags used in debug configuration
SET(CMAKE_CXX_FLAGS_DEBUG "${DEBUG_FLAGS}")
SET(CMAKE_C_FLAGS_DEBUG   "${DEBUG_FLAGS}")

# Flags used in release configuration
SET(CMAKE_CXX_FLAGS_RELEASE "${RELEASE_FLAGS}")
SET(CMAKE_C_FLAGS_RELEASE   "${RELEASE_FLAGS}")

# The linker has to be told to use a special memory layout 
# However this is different from the firmware versus the bootloader. 
# That's why it has been moved to the CMakeLists.txt file.

# Write a .map file, can be used to inspect code size
SET(FLAG_WRITE_MAP_FILE "-Wl,-Map=crownstone.map")

#SET(FLAG_REMOVE_UNWINDING_CODE "-Wl,--wrap,__aeabi_unwind_cpp_pr0")

SET(FLAG_REMOVE_UNWINDING_CODE "")

# Hardcode start address
#SET(FLAG_HARDCODE_STARTING_ADDRESS "-e0x2d000")
SET(FLAG_HARDCODE_STARTING_ADDRESS "")

# do not define above as multiple linker flags, or else you will get redefines of MEMORY etc.
SET(CMAKE_EXE_LINKER_FLAGS "${PATH_FILE_MEMORY} ${FILE_MEMORY_LAYOUT} ${CMAKE_EXE_LINKER_FLAGS} ${FLOAT_FLAGS} ${FLAG_WRITE_MAP_FILE} ${FLAG_REMOVE_UNWINDING_CODE} ${FLAG_HARDCODE_STARTING_ADDRESS}")

# We preferably want to run the cross-compiler tests without all the flags. This namely means we have to add for 
# example the object out of syscalls.c to the compilation, etc. Or, differently, have different flags for the 
# compiler tests. This is difficult to do!
#SET(CMAKE_C_FLAGS "-nostdlib")

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

