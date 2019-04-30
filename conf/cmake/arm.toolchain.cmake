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
	MESSAGE(STATUS "arm.toolchain.cmake: Load config file ${CONFIG_FILE}")
	INCLUDE(${CONFIG_FILE})
ELSE()
	MESSAGE(WARNING "arm.toolchain.cmake: Cannot find config file ${CONFIG_FILE}")
ENDIF()

MESSAGE(STATUS "arm.toolchain.cmake: Bluetooth name ${BLUETOOTH_NAME}")

#######################################################################################################################
# type of compiler we want to use, the COMPILER_TYPE can be empty if normal gcc and g++ compilers are intended
SET(COMPILER_TYPE_PREFIX ${COMPILER_TYPE})

#######################################################################################################################

# The extension .obj is just ugly, set it back to .o (does not work)
SET(CMAKE_CXX_OUTPUT_EXTENSION .o)

# Make cross-compiler easy to find (but we will use absolute paths anyway)
SET(PATH "${PATH}:${COMPILER_PATH}/bin")
MESSAGE(STATUS "arm.toolchain.cmake: PATH is set to: ${PATH}")

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

SET(DEFAULT_CXX_FLAGS       "-std=c++14 -fno-exceptions -fdelete-dead-exceptions -fno-unwind-tables -fno-non-call-exceptions")
#SET(DEFAULT_C_FLAGS         "-std=gnu99") // but why
SET(DEFAULT_C_FLAGS         "")
#SET(DEFAULT_C_AND_CXX_FLAGS "-mthumb -ffunction-sections -fdata-sections -g3 -Wall -Werror -fdiagnostics-color=always")
SET(DEFAULT_C_AND_CXX_FLAGS "-mthumb -ffunction-sections -fdata-sections -g3 -Wall -Werror -Wno-error=format -fdiagnostics-color=always -fno-builtin -flto")
#SET(DEFAULT_C_AND_CXX_FLAGS "-mthumb -ffunction-sections -fdata-sections -g3 -Wall -Werror -Wno-error=format -Wno-error=int-in-bool-context -fdiagnostics-color=always")

SET(ASM_OPTIONS "-x assembler-with-cpp")
SET(CMAKE_ASM_FLAGS "${CFLAGS} ${ASM_OPTIONS}" )

# Collect flags that have to do with optimization
# We are optimizing for SIZE for now. If size turns out to be abundant, enable -O3 optimization.
# Interesting options: -ffast-math, -flto (link time optimization), -fno-rtti
# Regarding size of the binary -ffast-math made a very small difference (4B), while -flto made a difference of almost 13kB.
# When adding -flto to C_FLAGS, you also need -fno-builtin, else it gives an error of multiple definitions of _exit().
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

MESSAGE(STATUS "arm.toolchain.cmake: C Compiler: ${CMAKE_C_COMPILER}")
MESSAGE(STATUS "arm.toolchain.cmake: Search for FindX files in ${CMAKE_MODULE_PATH}")

INCLUDE(crownstone.defs)

GET_DIRECTORY_PROPERTY( DirDefs DIRECTORY ${CMAKE_SOURCE_DIR} COMPILE_DEFINITIONS )

FOREACH(definition ${DirDefs})
	IF(VERBOSITY GREATER 4)
		MESSAGE(STATUS "arm.toolchain.cmake: Definition: " ${definition})
	ENDIF()
	IF(${definition} MATCHES "=$")
		IF(NOT ${definition} MATCHES "COMPILATION_TIME")
			MESSAGE(STATUS "arm.toolchain.cmake: Definition ${definition} is not defined" )
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

MESSAGE(STATUS "arm.toolchain.cmake: CXX flags: ${CMAKE_CXX_FLAGS} ${CMAKE_C_AND_CXX_FLAGS}")
MESSAGE(STATUS "arm.toolchain.cmake: C flags: ${CMAKE_C_FLAGS} ${CMAKE_C_AND_CXX_FLAGS}")

# __STARTUP_CONFIG is defined in gcc_startup_nrf52.S

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_AND_CXX_FLAGS} ${DEFINES}")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_AND_CXX_FLAGS} ${DEFINES}")


# Tell the linker that we use a special memory layout
#SET(FILE_MEMORY_LAYOUT "-TnRF51822-softdevice.ld")
#SET(PATH_FILE_MEMORY "-L${PROJECT_SOURCE_DIR}/conf")

MESSAGE(STATUS "BOOTLOADER: ${BOOTLOADER}")
IF (BOOTLOADER MATCHES 1)
	#SET(FILE_MEMORY_LAYOUT "-Tbootloader_nrf52832_xxAA.ld")
	SET(FILE_MEMORY_LAYOUT "-Tsecure_bootloader_gcc_nrf52.ld")
	SET(PATH_FILE_MEMORY "-L${CMAKE_SOURCE_DIR}/bootloader/")
ELSE()
	SET(FILE_MEMORY_LAYOUT "-Tgeneric_gcc_nrf52.ld")
	#SET(PATH_FILE_MEMORY "-L${NRF5_DIR}/config/nrf52832/armgcc/")
	SET(PATH_FILE_MEMORY "-L${CMAKE_SOURCE_DIR}/include/third/nrf/")
ENDIF()
SET(PATH_FILE_MEMORY "${PATH_FILE_MEMORY} -L${NRF5_DIR}/modules/nrfx/mdk/")

# http://public.kitware.com/Bug/view.php?id=12652
# CMake does send the compiler flags also to the linker

SET(FLAG_WRITE_MAP_FILE "-Wl,-Map,prog.map")
#SET(FLAG_REMOVE_UNWINDING_CODE "-Wl,--wrap,__aeabi_unwind_cpp_pr0")
SET(FLAG_REMOVE_UNWINDING_CODE "")

# Hardcode start address
#SET(FLAG_HARDCODE_STARTING_ADDRESS "-e0x2d000")
SET(FLAG_HARDCODE_STARTING_ADDRESS "")

# do not define above as multiple linker flags, or else you will get redefines of MEMORY etc.
SET(CMAKE_EXE_LINKER_FLAGS "${PATH_FILE_MEMORY} ${FILE_MEMORY_LAYOUT} -Wl,--gc-sections ${CMAKE_EXE_LINKER_FLAGS} ${FLOAT_FLAGS} ${FLAG_WRITE_MAP_FILE} ${FLAG_REMOVE_UNWINDING_CODE} ${FLAG_HARDCODE_STARTING_ADDRESS}")

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
MESSAGE(STATUS "arm.toolchain.cmake: Add include path: ${CMAKE_INCLUDE_PATH}")

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

