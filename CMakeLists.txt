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

IF(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
ENDIF(COMMAND cmake_policy)

SET(TEST_COMPILERS CXX C)

# Set application shorthand
SET(APPLICATION_SHORTHAND "crownstone")

# Set name of project
SET(PROJECT_NAME ${APPLICATION_SHORTHAND} ${TEST_COMPILERS})

# Set the main file to use for this application
SET(MAIN_FILE "main_${APPLICATION_SHORTHAND}.cpp")

# Start a project
PROJECT(${PROJECT_NAME})

# The directory with some of the FindXXX modules
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
MESSAGE(STATUS "Search for FindX files in ${CMAKE_SOURCE_DIR}/cmake")

#######################################################################################################################
# Include your own FindX cmake files and options in the following file!
#######################################################################################################################

SET(CONFIGURATION_FILE "CMakeBuild.config")
IF(EXISTS ${CMAKE_SOURCE_DIR}/${CONFIGURATION_FILE})
	file(STRINGS ${CMAKE_SOURCE_DIR}/${CONFIGURATION_FILE} ConfigContents)
	foreach(NameAndValue ${ConfigContents})
		# Strip leading spaces
		string(REGEX REPLACE "^[ ]+" "" NameAndValue ${NameAndValue})
		# Find variable name
		string(REGEX MATCH "^[^=]+" Name ${NameAndValue})
		# Find the value
		string(REPLACE "${Name}=" "" Value ${NameAndValue})
		# Set the variable
		set(${Name} "${Value}")
	endforeach()
else()
	MESSAGE(FATAL_ERROR "Could not find file ${CONFIGURATION_FILE}, copy from ${CONFIGURATION_FILE}.default and adjust!")
endif()

#######################################################################################################################

MESSAGE(STATUS "Set application start address in linker script to ${APPLICATION_START_ADDRESS}")
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/conf/nRF51822-softdevice.ld.in ${CMAKE_SOURCE_DIR}/conf/nRF51822-softdevice.ld)

# Libraries (math, stdc++ libraries, etc., note that we use the size-constrained *_s versions!)
SET(LIBS "-lsupc++ -lm -lc_s -lstdc++_s")

# Some debug information
MESSAGE(STATUS "\"${PROJECT_NAME}\" uses the following CXX flags: \"${CMAKE_CXX_FLAGS}\" (can be empty)")
MESSAGE(STATUS "Linked libraries: ${LIBS}")

# Source directory
SET(SOURCE_DIR "src")
SET(INCLUDE_DIR "include" "include/common")

# essential, code doesn't run without this file !!!
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/nRF51822.c")

# Add a single 
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/error.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/ble_error.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/syscalls.c")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/utils.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/common/storage.cpp")
#LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/common/timer.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/serial.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/nrf_pwm.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/nrf_adc.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/nrf_rtc.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/characteristics/Dimming.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/characteristics/ScanResult.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/BluetoothLE.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/IndoorLocalisationService.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/GeneralService.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/PowerService.cpp")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/${MAIN_FILE}")

# Add all header files without exception
#FILE(GLOB FOLDER_HEADER ${INCLUDE_DIR}/*.h ${INCLUDE_DIR}/*.hh ${INCLUDE_DIR}/*.hpp)

# We add the source files explicitly. This is recommended in the cmake system and will also force us all the time to
# consider the size of the final binary. Do not include things, if not necessary!
LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/Source/ble/ble_advdata.c")
#LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/Source/ble/ble_advdata_parser.c")
#LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/Source/app_common/app_timer.c")
LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/Source/app_common/app_scheduler.c")
LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/Source/app_common/pstorage.c")
LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/Source/nrf_delay/nrf_delay.c")
LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/Source/sd_common/softdevice_handler.c")

FOREACH(nordic_file ${NORDIC_SOURCE})
	MESSAGE(STATUS "Include Nordic file: " ${nordic_file})
ENDFOREACH()

LIST(APPEND FOLDER_SOURCE ${NORDIC_SOURCE})

# For Visual Studio group the files
SOURCE_GROUP("Source files" FILES ${FOLDER_SOURCE})
SOURCE_GROUP("Header files" FILES ${FOLDER_HEADER})

INCLUDE_DIRECTORIES(${INCLUDE_DIR})

# Add to the -I flags
INCLUDE_DIRECTORIES("${NRF51822_DIR}/Include")
INCLUDE_DIRECTORIES("${NRF51822_DIR}/Include/app_common")
INCLUDE_DIRECTORIES("${NRF51822_DIR}/Include/ble")
#INCLUDE_DIRECTORIES("${NRF51822_DIR}/Include/ble/softdevice")
INCLUDE_DIRECTORIES("${NRF51822_DIR}/Include/ble/ble_services")
INCLUDE_DIRECTORIES("${NRF51822_DIR}/Include/gcc")

# Recent changes in directory structure with new SDK
IF(${SOFTDEVICE_SERIES} STREQUAL "separate_softdevice_source")
	MESSAGE(STATUS "Add s110 directory from SDK")
	INCLUDE_DIRECTORIES("${NRF51822_DIR}/Include/s110")
ENDIF()
INCLUDE_DIRECTORIES("${NRF51822_DIR}/Include/sd_common")

INCLUDE_DIRECTORIES("${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_API}/include")

# Show which directories are actually included to the user
GET_PROPERTY(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
FOREACH(dir ${dirs})
	MESSAGE(STATUS "Add to the include directories: '${dir}'")
ENDFOREACH()

# Set up our main executable.
IF(FOLDER_SOURCE STREQUAL "")
	MESSAGE(FATAL_ERROR "No source code files found. Please add something")
ELSE()
	ADD_EXECUTABLE(${PROJECT_NAME} ${FOLDER_SOURCE} ${FOLDER_HEADER} ${OBJECT_FILES})
	
	# https://stackoverflow.com/questions/5278444/adding-a-custom-command-with-the-file-name-as-a-target
	ADD_CUSTOM_COMMAND(OUTPUT ${PROJECT_NAME}.tmp
		COMMAND ${CMAKE_OBJCOPY_OVERLOAD} -j .text -j .data -O binary ${PROJECT_NAME} ${PROJECT_NAME}.tmp
		COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_NAME}.tmp ${PROJECT_NAME}.bin
		COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_NAME} ${PROJECT_NAME}.elf
		COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
		COMMAND ${CMAKE_COMMAND} -E remove ${PROJECT_NAME}
		COMMAND ${CMAKE_COMMAND} -E remove ${PROJECT_NAME}.tmp
		DEPENDS ${PROJECT_NAME}
		COMMENT "Object copy ${PROJECT_NAME} to ${PROJECT_NAME}.bin")

	ADD_CUSTOM_TARGET(analyze
		COMMAND echo
		COMMAND echo "*** Show headers"
		COMMAND ${CMAKE_READELF} --headers ${PROJECT_NAME}.elf
		#COMMAND ${CMAKE_OBJDUMP} -h ${PROJECT_NAME}.elf
		COMMAND echo
		COMMAND echo "*** Show largest symbols / functions"
		COMMAND ${CMAKE_NM} --print-size --size-sort --radix=d ${PROJECT_NAME}.elf | tail -n20
		COMMAND echo
		COMMAND echo "*** Show size of sections"
		COMMAND ${CMAKE_SIZE} -A -d ${PROJECT_NAME}.elf
		DEPENDS ${PROJECT_NAME}.tmp
		COMMENT "Read headers ${PROJECT_NAME}.elf"
		)

	ADD_CUSTOM_TARGET(${PROJECT_NAME}.bin ALL DEPENDS ${PROJECT_NAME}.tmp)

	SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${PROJECT_NAME}.bin)

	TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIBS})
	INSTALL(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
ENDIF()

