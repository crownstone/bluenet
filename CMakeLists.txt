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

# Makes compilation go wrong first time
SET(TEST_COMPILERS CXX C)
#SET(TEST_COMPILERS "")

# Set application shorthand
SET(APPLICATION_SHORTHAND "crownstone")

# Set name of project
SET(PROJECT_NAME ${APPLICATION_SHORTHAND})

# Set the main file to use for this application
SET(MAIN_FILE "cs_main_${APPLICATION_SHORTHAND}.cpp")

# Start a project
PROJECT(${PROJECT_NAME} ${TEST_COMPILERS})

# The directory with some of the FindXXX modules
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
MESSAGE(STATUS "Search for FindX files in ${CMAKE_SOURCE_DIR}/cmake")

#######################################################################################################################
# Include your own FindX cmake files and options in the following file!
#######################################################################################################################

SET(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/CMakeConfig.cmake")
#SET(CONFIG_FILE "CMakeBuild.config")
IF(EXISTS ${CONFIG_FILE})
	file(STRINGS ${CONFIG_FILE} ConfigContents)
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
	MESSAGE(FATAL_ERROR "Could not find file ${CONFIG_FILE}, copy from ${CONFIG_FILE}.default and adjust!")
endif()

#######################################################################################################################

MESSAGE(STATUS "Set application start address in linker script to ${APPLICATION_START_ADDRESS}")
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/conf/nRF51822-softdevice.ld.in ${CMAKE_SOURCE_DIR}/conf/nRF51822-softdevice.ld)

# Libraries (math, stdc++ libraries, etc., note that we use the size-constrained *_s versions!)
#SET(LIBRARY_MATH "-lm")
#SET(LIBRARY_RUNTIME "-lsupc++")

SET(LIBS "-lstdc++_s ${LIBRARY_RUNTIME} ${LIBRARY_MATH} -lc_s")

# Some debug information
MESSAGE(STATUS "\"${PROJECT_NAME}\" uses the following CXX flags: \"${CMAKE_CXX_FLAGS}\" (can be empty)")
MESSAGE(STATUS "Linked libraries: ${LIBS}")

# Source directory
SET(SOURCE_DIR "src")

# Add separately include directories if we are to lazy to specify e.g. "#include <protocol/rbc_mesh.h>"
SET(INCLUDE_DIR "include" "include/third" "include/third/nrf" "include/third/protocol")

SET(BOOTLOADER "_bootloader_.hex")

# essential, code doesn't run without this file !!!
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/cs_sysNrf51.c")

# Add a single
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_Error.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_BleError.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_Syscalls.c")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/cfg/cs_Config.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_Storage.cpp")
#LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/structs/buffer/cs_DifferentialBuffer.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_Serial.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_PWM.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_ADC.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_LPComp.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_RNG.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_Dimming.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/structs/cs_ScanResult.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/structs/cs_TrackDevices.cpp")
#LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/structs/cs_StreamBuffer.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/events/cs_EventDispatcher.cpp")
IF(${GENERAL_SERVICE} STRGREATER "0")
	IF(${CHAR_MESHING} STRGREATER "0")
		LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/protocol/cs_MeshControl.cpp")
	ENDIF()
ENDIF()
#LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/cs_BluetoothLE.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_UUID.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_Stack.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_Service.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_Characteristic.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_Handlers.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_Softdevice.c")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_Sensors.cpp")
IF(${INDOOR_SERVICE} STRGREATER "0")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/cs_IndoorLocalisationService.cpp")
ENDIF()
IF(${GENERAL_SERVICE} STRGREATER "0")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/cs_GeneralService.cpp")
ENDIF()
IF(${POWER_SERVICE} STRGREATER "0")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/cs_PowerService.cpp")
ENDIF()

# should be only when creating iBeacon
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_iBeacon.cpp")

IF(${CHAR_MESHING} STRGREATER "0")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/protocol/cs_Mesh.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/third/protocol/mesh_srv.c")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/third/protocol/radio_control.c")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/third/protocol/rbc_mesh.c")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/third/protocol/timer_control.c")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/third/protocol/timeslot_handler.c")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/third/protocol/transport_control.c")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/third/protocol/trickle.c")
endif()

IF(${CHAR_MESHING} STRGREATER "0")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/third/nrf/app_timer.c")
else()
	LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/Source/app_common/app_timer.c")
endif()

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/${MAIN_FILE}")

FOREACH(crownstone_file ${FOLDER_SOURCE})
	MESSAGE(STATUS "Include Crownstone file: " ${crownstone_file})
ENDFOREACH()

# Add all header files without exception
#FILE(GLOB FOLDER_HEADER ${INCLUDE_DIR}/*.h ${INCLUDE_DIR}/*.hh ${INCLUDE_DIR}/*.hpp)

# We add the source files explicitly. This is recommended in the cmake system and will also force us all the time to
# consider the size of the final binary. Do not include things, if not necessary!
LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/Source/ble/ble_advdata.c")
#LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/Source/ble/ble_advdata_parser.c")
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
		COMMAND ${CMAKE_OBJCOPY_OVERLOAD} -j .text -j .data -O ihex ${PROJECT_NAME} ${PROJECT_NAME}.hex
		COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_NAME}.tmp ${PROJECT_NAME}.bin
		COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_NAME} ${PROJECT_NAME}.elf
		COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
		COMMAND ${CMAKE_COMMAND} -E remove ${PROJECT_NAME}
		COMMAND ${CMAKE_COMMAND} -E remove ${PROJECT_NAME}.tmp
		DEPENDS ${PROJECT_NAME}
		COMMENT "Object copy ${PROJECT_NAME} to ${PROJECT_NAME}.bin")

	ADD_CUSTOM_TARGET(combine
		COMMAND echo
		COMMAND echo "*** Use srec_cat to create one combined softdevice+app file"
		COMMAND echo
		COMMAND echo "srec_cat ${SOFTDEVICE_DIR}/${SOFTDEVICE}_softdevice.hex -intel bootloader.hex -intel ${PROJECT_NAME}.bin -binary -offset ${APPLICATION_START_ADDRESS} -o combined.hex -intel"
		COMMAND srec_cat ${SOFTDEVICE_DIR}/${SOFTDEVICE}_softdevice.hex -intel bootloader.hex -intel ${PROJECT_NAME}.bin -binary -offset ${APPLICATION_START_ADDRESS} -o combined.hex -intel
		DEPENDS ${PROJECT_NAME}.tmp
		COMMENT "Combine as one hex file"
		)

	ADD_CUSTOM_TARGET(analyze
		COMMAND echo
		COMMAND echo "*** Show headers"
		COMMAND ${CMAKE_READELF} --headers ${PROJECT_NAME}.elf
		#COMMAND ${CMAKE_OBJDUMP} -h ${PROJECT_NAME}.elf
		COMMAND echo
		COMMAND echo "*** Show largest symbols / functions"
		COMMAND ${CMAKE_NM} --print-size --size-sort --radix=d ${PROJECT_NAME}.elf
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

