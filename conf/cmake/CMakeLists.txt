#######################################################################################################################
# The build systems uses CMake. All the automatically generated code falls under the Lesser General Public License
# (LGPL GNU v3), the Apache License, or the MIT license, your choice.
#
# Author:	 Anne C. van Rossum (Distributed Organisms B.V.)
# Date: 	 Oct 28, 2013
#
# Copyright © 2013 Anne C. van Rossum <anne@dobots.nl>
#######################################################################################################################

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# Set application shorthand
SET(APPLICATION_SHORTHAND "crownstone")

# Set name of project
SET(PROJECT_NAME ${APPLICATION_SHORTHAND})

# Set the main file to use for this application
SET(MAIN_FILE "cs_main_${APPLICATION_SHORTHAND}.cpp")

# Start a project
PROJECT(${PROJECT_NAME}) # ${TEST_COMPILERS})

# The directory with some of the FindXXX modules
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/conf;${CMAKE_SOURCE_DIR}/conf/cmake")
MESSAGE(STATUS "CMakeLists: Search for FindX files in ${CMAKE_MODULE_PATH}")

#######################################################################################################################

SET(VERBOSITY                                "4"           CACHE STRING "Verbosity")
SET(HOST_TARGET                              "ON"          CACHE BOOL "Compile for host platform")
SET(GIT_BRANCH                               "unset"       CACHE STRING "Current git branch")
#SET(GIT_HASH                                 "unset"       CACHE STRING "Current git hash")
SET(COMPILATION_DAY                          "unset"       CACHE STRING "Day of compilation")
SET(COMPILATION_TIME                         "unset"       CACHE STRING "Time of day of compilation")
SET(NRF51822_DIR                             "unset"       CACHE STRING "The nRF5 sdk directory")

#######################################################################################################################
# Show variables
#######################################################################################################################

MESSAGE(STATUS "CMakeLists: Set verbosity to level: ${VERBOSITY}")
MESSAGE(STATUS "CMakeLists: Git branch: ${GIT_BRANCH}")
#MESSAGE(STATUS "CMakeLists: Git hash: ${GIT_HASH}")
MESSAGE(STATUS "CMakeLists: Day of compilation: ${COMPILATION_DAY}")
MESSAGE(STATUS "CMakeLists: Time of day of compilation: ${COMPILATION_TIME}")
MESSAGE(STATUS "CMakeLists: Compiler: ${CMAKE_C_COMPILER}")

#######################################################################################################################
# Include your own FindX cmake files and options in the following file!
#######################################################################################################################


#######################################################################################################################

MESSAGE(STATUS "CMakeLists: Set application start address in linker script to ${APPLICATION_START_ADDRESS}")
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/conf/nRF51822-softdevice.ld.in ${CMAKE_SOURCE_DIR}/conf/nRF51822-softdevice.ld)

# Libraries (math, stdc++ libraries, etc.) 
# Note! We CANNOT use the size-constrained *_s versions! anymore
# We don't seem to use any libraries anymore... except for including everything that comes with -std=c+11
#SET(LIBS "-lc ${LIBRARY_RUNTIME} ${LIBRARY_MATH} -lstdc++")
SET(LIBS "")

# Some debug information
IF(VERBOSITY GREATER 4)
	MESSAGE(STATUS "CMakeLists: \"${PROJECT_NAME}\" uses the following CXX flags: \"${CMAKE_CXX_FLAGS}\" (can be empty)")
	MESSAGE(STATUS "CMakeLists: Linked libraries: ${LIBS}")
ENDIF()

# Source directory
SET(SOURCE_DIR "src")

# Add separately include directories if we are to lazy to specify e.g. "#include <protocol/rbc_mesh.h>"
SET(INCLUDE_DIR "include" "include/third" "include/third/nrf" "include/ble")

SET(BOOTLOADER "_bootloader_.hex")

IF((DEFINED TEST_TARGET) AND "${TEST_TARGET}" STREQUAL "nrf5")
	MESSAGE(STATUS "Run test target. This will include source files as necessary")
ELSE()
	INCLUDE(crownstone.src)

	IF(BUILD_MESHING AND "${BUILD_MESHING}" STRGREATER "0")
		LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/third/nrf/app_timer.c")
	else()
		LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/libraries/timer/app_timer.c")
	endif()

	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/third/nrf/device_manager_peripheral.c")

	LIST(APPEND NORDIC_SOURCE "${SOURCE_DIR}/third/nrf/nrf_drv_common.c")

	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/${MAIN_FILE}")

	IF(VERBOSITY GREATER 4)
		FOREACH(crownstone_file ${FOLDER_SOURCE})
			MESSAGE(STATUS "Include Crownstone file: " ${crownstone_file})
		ENDFOREACH()
	ENDIF()

	# Add all header files without exception
	#FILE(GLOB FOLDER_HEADER ${INCLUDE_DIR}/*.h ${INCLUDE_DIR}/*.hh ${INCLUDE_DIR}/*.hpp)

	# We add the source files explicitly. This is recommended in the cmake system and will also force us all the time to
	# consider the size of the final binary. Do not include things, if not necessary!
	LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/ble/common/ble_advdata.c")
	#LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/ble/common/ble_advdata_parser.c")
	#LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/ble/device_manager/device_manager_peripheral.c")
	#LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/ble/device_manager/device_manager_central.c")
	# LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/drivers_nrf/common/nrf_drv_common.c")
	LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/drivers_nrf/hal/nrf_saadc.c")
	LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/drivers_nrf/comp/nrf_drv_comp.c")
	LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/drivers_nrf/gpiote/nrf_drv_gpiote.c")
	LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/drivers_nrf/delay/nrf_delay.c")
	LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/drivers_nrf/ppi/nrf_drv_ppi.c")
	LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/drivers_nrf/pstorage/pstorage.c")
	LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/drivers_nrf/timer/nrf_drv_timer.c")
	LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/drivers_nrf/saadc/nrf_drv_saadc.c")
	LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/libraries/scheduler/app_scheduler.c")
	LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/libraries/pwm/app_pwm.c")
	LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/libraries/timer/app_timer_appsh.c")
	LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/libraries/util/app_util_platform.c")
	LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/softdevice/common/softdevice_handler/softdevice_handler.c")
	LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/softdevice/common/softdevice_handler/softdevice_handler_appsh.c")
	LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/drivers_nrf/hal/nrf_nvmc.c")

	IF(VERBOSITY GREATER 4)
		FOREACH(nordic_file ${NORDIC_SOURCE})
			MESSAGE(STATUS "Include Nordic file: " ${nordic_file})
		ENDFOREACH()
	ENDIF()

	LIST(APPEND FOLDER_SOURCE ${NORDIC_SOURCE})

	# For Visual Studio group the files
	SOURCE_GROUP("Source files" FILES ${FOLDER_SOURCE})
	SOURCE_GROUP("Header files" FILES ${FOLDER_HEADER})

	INCLUDE_DIRECTORIES(${INCLUDE_DIR})

	# Add to the -I flags
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/ble/common")
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/ble/ble_services")
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/ble/device_manager")
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/device/")
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/drivers_nrf/common/")
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/drivers_nrf/gpiote/")
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/drivers_nrf/delay/")
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/drivers_nrf/hal/")
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/drivers_nrf/ppi/")
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/drivers_nrf/pstorage/")
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/drivers_nrf/timer/")
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/drivers_nrf/saadc/")
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/drivers_nrf/comp/")
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/libraries/pwm")
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/libraries/util/")
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/libraries/timer/")
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/libraries/scheduler/")
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/libraries/trace/")
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/softdevice/common/softdevice_handler/")
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/softdevice/s132/headers/")
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/toolchain/")
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/toolchain/gcc")
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/toolchain/CMSIS/Include")

	INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/softdevice/s${SOFTDEVICE_SERIES}/headers")

	IF(DEFINED SOFTDEVICE_DIR AND DEFINED SOFTDEVICE_DIR_API)
		INCLUDE_DIRECTORIES("${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_API}")
	ENDIF()

ENDIF()

# Show which directories are actually included to the user
GET_PROPERTY(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
IF(VERBOSITY GREATER 4)
	FOREACH(dir ${dirs})
		MESSAGE(STATUS "CMakeLists: Add to the include directories: '${dir}'")
	ENDFOREACH()
ENDIF()

IF(NOT (DEFINED SOFTDEVICE_DIR AND DEFINED SOFTDEVICE_DIR_HEX))
	SET(${SOFTDEVICE_DIR}, "${NRF51822_DIR}/components/softdevice/s${SOFTDEVICE_SERIES}/")
	SET(${SOFTDEVICE_DIR_HEX}, "hex")
ENDIF()

IF((DEFINED TEST_TARGET) AND "${TEST_TARGET}" STREQUAL "nrf5")
	MESSAGE(STATUS "CMakeLists: Add testing files")

	ENABLE_TESTING()

	#	ADD_SUBDIRECTORY(test/nrf5)

	INCLUDE(test/nrf5/CMakeLists.txt)
ELSE()

	# Set up our main executable.
	IF(FOLDER_SOURCE STREQUAL "")
		MESSAGE(FATAL_ERROR "No source code files found. Please add something")
	ELSE()
		ADD_EXECUTABLE(${PROJECT_NAME} ${FOLDER_SOURCE} ${FOLDER_HEADER} ${OBJECT_FILES} ${PROJECT_NAME}.tmp)

		# https://stackoverflow.com/questions/5278444/adding-a-custom-command-with-the-file-name-as-a-target
		ADD_CUSTOM_COMMAND(OUTPUT ${PROJECT_NAME}.tmp
			COMMAND ${CMAKE_OBJCOPY_OVERLOAD} -j .text -j .data -O binary ${PROJECT_NAME} ${PROJECT_NAME}.tmp
			COMMAND ${CMAKE_OBJCOPY_OVERLOAD} -j .text -j .data -O ihex ${PROJECT_NAME} ${PROJECT_NAME}.hex
			COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_NAME}.tmp ${PROJECT_NAME}.bin
			COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_NAME} ${PROJECT_NAME}.elf
			COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
			COMMAND stat ${PROJECT_NAME}.bin | grep -i size | tr -d ':' |  tr -s ' ' | cut -f3 -d ' ' | xargs printf "** Size: %s bytes - totally available: 512kB - 110kB {softdevice} = 412kB. In dual bank: 206kB"
			COMMAND echo
			DEPENDS ${PROJECT_NAME}
			COMMENT "Object copy ${PROJECT_NAME} to ${PROJECT_NAME}.bin")

		ADD_CUSTOM_TARGET(generate-dat
			COMMAND echo
			COMMAND echo "** Use srec_cat to create the INIT .dat file for the DFU"
			COMMAND echo
			COMMAND echo "srec_cat ${PROJECT_NAME}.hex -intel -exclude 0x3FC00 0x3FC20 -generate 0x3FC00 0x3FC04 -l-e-constant 0x01 4 -generate 0x3FC04 0x3FC08 -l-e-constant 0x00 4 -generate 0x3FC08 0x3FC0C -l-e-constant 0xFE 4 -generate 0x3FC0C 0x3FC20 -constant 0x00 -o ${PROJECT_NAME}.dat -intel"
			COMMAND srec_cat ${PROJECT_NAME}.hex -intel -exclude 0x3FC00 0x3FC20 -generate 0x3FC00 0x3FC04 -l-e-constant 0x01 4 -generate 0x3FC04 0x3FC08 -l-e-constant 0x00 4 -generate 0x3FC08 0x3FC0C -l-e-constant 0xFE 4 -generate 0x3FC0C 0x3FC20 -constant 0x00 -o ${PROJECT_NAME}.dat -intel
			DEPENDS ${PROJECT_NAME}.tmp
			COMMENT "Generate the INIT .dat file for the DFU"
			)

		ADD_CUSTOM_TARGET(combine
			COMMAND echo
			COMMAND echo "** Use srec_cat to create one combined softdevice+app file"
			COMMAND echo
			COMMAND echo "srec_cat ${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_HEX}/${SOFTDEVICE}_softdevice.hex -intel bootloader.hex -intel ${PROJECT_NAME}.bin -binary -offset ${APPLICATION_START_ADDRESS} -o combined.hex -intel"
			COMMAND srec_cat ${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_HEX}/${SOFTDEVICE}_softdevice.hex -intel bootloader.hex -intel ${PROJECT_NAME}.bin -binary -offset ${APPLICATION_START_ADDRESS} -o combined.hex -intel
			DEPENDS ${PROJECT_NAME}.tmp
			COMMENT "Combine as one hex file"
			)

		ADD_CUSTOM_TARGET(analyze
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
			DEPENDS ${PROJECT_NAME}.tmp
			COMMENT "Read headers ${PROJECT_NAME}.elf"
			)

		SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${PROJECT_NAME}.bin)
		
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIBS})
		INSTALL(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
	ENDIF()

ENDIF()

