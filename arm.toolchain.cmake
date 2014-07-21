#Check http://www.cmake.org/Wiki/CMake_Cross_Compiling

# Set to Generic, or tests will fail (they will fail anyway)
SET(CMAKE_SYSTEM_NAME Generic)

# Tell we want to cross-compile
SET(CMAKE_SYSTEM_VERSION 1)
SET(CMAKE_SYSTEM_PROCESSOR arm)
SET(CMAKE_CROSSCOMPILING 1)

# Load compiler options from configuration file
INCLUDE(${PROJECT_SOURCE_DIR}/CMakeConfig.cmake)

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
SET(CMAKE_OBJCOPY	${COMPILER_PATH}/bin/${COMPILER_TYPE}-objcopy)
SET(CMAKE_SIZE		${COMPILER_PATH}/bin/${COMPILER_TYPE}-size)
SET(CMAKE_NM		${COMPILER_PATH}/bin/${COMPILER_TYPE}-nm)

# There is a bug in CMAKE_OBJCOPY, it doesn't exist on execution for the first time
SET(CMAKE_OBJCOPY_OVERLOAD	${COMPILER_PATH}/bin/${COMPILER_TYPE}-objcopy)

SET(CMAKE_CXX_FLAGS                              "-std=c++11 -fno-exceptions"                    CACHE STRING "c++ flags")
SET(CMAKE_C_FLAGS                                "-std=gnu99"                                    CACHE STRING "c flags")
SET(CMAKE_SHARED_LINKER_FLAGS                    ""                                              CACHE STRING "shared linker flags")
SET(CMAKE_MODULE_LINKER_FLAGS                    ""                                              CACHE STRING "module linker flags")
SET(CMAKE_EXE_LINKER_FLAGS                       "-Wl,-z,nocopyreloc"                            CACHE STRING "executable linker flags")
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS            "") 
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS          "") 

# Collect flags that have to do with optimization, optimize for size for now
SET(OPTIMIZE "-Os -fomit-frame-pointer -ffast-math")

# Collect flags that are used in the code, as macros
SET(DEFINES "-MMD -DNRF51 -DEPD_ENABLE_EXTRA_RAM -DARDUINO=100 -DE_STICKY_v1 -DNRF51_USE_SOFTDEVICE=1 -DUSE_RENDER_CONTEXT -DSYSCALLS -DUSING_FUNC")

# Set the compiler flags
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -Wall ${OPTIMIZE} -mcpu=cortex-m0 -mthumb -ffunction-sections -fdata-sections ${DEFINES}")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -Wall ${OPTIMIZE} -mcpu=cortex-m0 -mthumb -ffunction-sections -fdata-sections ${DEFINES}")

# Tell the linker that we use a special memory layout
SET(FILE_MEMORY_LAYOUT "-TnRF51822-softdevice.ld") 
SET(PATH_FILE_MEMORY "-L${PROJECT_SOURCE_DIR}/conf") 

# http://public.kitware.com/Bug/view.php?id=12652
# CMake does send the compiler flags also to the linker

# do not define above as multiple linker flags, or else you will get redefines of MEMORY etc.
SET(CMAKE_EXE_LINKER_FLAGS "${PATH_FILE_MEMORY} ${FILE_MEMORY_LAYOUT} -Wl,--gc-sections ${CMAKE_SHARED_LINKER_FLAGS}")

# We preferably want to run the cross-compiler tests without all the flags. This namely means we have to add for example the object out of syscalls.c to the compilation, etc. Or, differently, have different flags for the compiler tests. This is difficult to do!
#SET(CMAKE_C_FLAGS "-nostdlib")

SET(CMAKE_C_COMPILER_WORKS 1)
SET(CMAKE_CXX_COMPILER_WORKS 1)

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

