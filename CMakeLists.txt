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

SET(VERBOSITY                                "${VERBOSITY}"                   CACHE STRING "4" FORCE)

#######################################################################################################################
# Include your own FindX cmake files and options in the following file!
#######################################################################################################################


#######################################################################################################################

MESSAGE(STATUS "Set application start address in linker script to ${APPLICATION_START_ADDRESS}")
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/conf/nRF51822-softdevice.ld.in ${CMAKE_SOURCE_DIR}/conf/nRF51822-softdevice.ld)

# Libraries (math, stdc++ libraries, etc., note that we use the size-constrained *_s versions!)
#SET(LIBRARY_MATH "-lm")
#SET(LIBRARY_RUNTIME "-lsupc++")

#SET(LIBS "-lstdc++_s ${LIBRARY_RUNTIME} ${LIBRARY_MATH} -lc_s")
SET(LIBS "-lstdc++ ${LIBRARY_RUNTIME} ${LIBRARY_MATH} -lc")

# Some debug information
IF(VERBOSITY GREATER 4)
	MESSAGE(STATUS "\"${PROJECT_NAME}\" uses the following CXX flags: \"${CMAKE_CXX_FLAGS}\" (can be empty)")
	MESSAGE(STATUS "Linked libraries: ${LIBS}")
ENDIF()

# Source directory
SET(SOURCE_DIR "src")

# Add separately include directories if we are to lazy to specify e.g. "#include <protocol/rbc_mesh.h>"
SET(INCLUDE_DIR "include" "include/third" "include/third/nrf" "include/ble")

SET(BOOTLOADER "_bootloader_.hex")

# essential, code doesn't run without this file !!!
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/cs_sysNrf51.c")

LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/cfg/cs_Boards.c")

# Add a single
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_Error.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_BleError.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_Syscalls.c")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/storage/cs_Settings.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/storage/cs_State.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/storage/cs_StorageHelper.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_Storage.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_Serial.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_PWM.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_ADC.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_COMP.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_RNG.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/structs/cs_ScanResult.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/structs/cs_TrackDevices.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/structs/cs_ScheduleEntries.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/structs/cs_PowerSamples.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/events/cs_EventDispatcher.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_UUID.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_Stack.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_Service.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_Characteristic.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_Handlers.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_Softdevice.c")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_DoBotsManufac.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_ServiceData.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_EncryptionHandler.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_CommandHandler.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_Tracker.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_Scanner.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_Scheduler.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_FactoryReset.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_EnOceanHandler.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_TemperatureGuard.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/cs_DeviceInformationService.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/cs_CrownstoneService.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/cs_SetupService.cpp")
IF(${INDOOR_SERVICE} STRGREATER "0")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/cs_IndoorLocalisationService.cpp")
ENDIF()
IF(${GENERAL_SERVICE} STRGREATER "0")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/cs_GeneralService.cpp")
ENDIF()
IF(${POWER_SERVICE} STRGREATER "0")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/cs_PowerService.cpp")
ENDIF()
IF(${SCHEDULE_SERVICE} STRGREATER "0")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/services/cs_ScheduleService.cpp")
ENDIF()


# IF(${DEVICE_TYPE} STREQUAL DEVICE_CROWNSTONE_PLUG OR ${DEVICE_TYPE} STREQUAL DEVICE_CROWNSTONE_BUILTIN)
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_PowerSampling.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_Switch.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/processing/cs_Watchdog.cpp")
# ENDIF()

# should be only when creating iBeacon
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_iBeacon.cpp")

IF(${EDDYSTONE} STRGREATER "0")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/ble/cs_Eddystone.cpp")
ENDIF()

IF(${MESHING} STRGREATER "0" AND ${BUILD_MESHING} STREQUAL "0")
	MESSAGE(FATAL_ERROR "Need to set BUILD_MESHING=1 if MESHING should be enabled!")
ENDIF()

IF(${BUILD_MESHING} STRGREATER "0")

	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/mesh/cs_MeshControl.cpp")
	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/mesh/cs_Mesh.cpp")

	IF(IS_DIRECTORY "${MESH_DIR}")
		MESSAGE(STATUS "Use directory ${MESH_DIR} for the mesh")
	ELSE()
		MESSAGE(FATAL_ERROR "Directory \"${MESH_DIR}\" does not exist! Clone https://github.com/mrquincle/nRF51-ble-bcast-mesh and switch to nrf52 branch.")
	ENDIF()
	EXECUTE_PROCESS(
		COMMAND git rev-parse --abbrev-ref HEAD
		WORKING_DIRECTORY ${MESH_DIR}
		OUTPUT_VARIABLE MESH_BRANCH
		OUTPUT_STRIP_TRAILING_WHITESPACE)

	IF(MESH_BRANCH STREQUAL "bluenet")
		MESSAGE(STATUS "Using bluenet branch for the Mesh")
	ELSE()
		MESSAGE(FATAL_ERROR "Switch ${MESH_DIR} to bluenet branch. The master branch is not bluenet-compatible yet.")
	ENDIF()

	SET(INCLUDE_DIR "${INCLUDE_DIR}" "${MESH_DIR}" "${MESH_DIR}/include")
	MESSAGE(STATUS "Include dirs: " ${INCLUDE_DIR})

	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/event_handler.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/fifo.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/handle_storage.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/mesh_aci.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/mesh_gatt.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/mesh_packet.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/notification_buffer.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/radio_control.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/rand.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/rbc_mesh.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/timer.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/timeslot.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/timer_scheduler.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/transport_control.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/trickle.c")
	LIST(APPEND FOLDER_SOURCE "${MESH_DIR}/src/version_handler.c")

ENDIF()

IF(${BUILD_MESHING} STRGREATER "0")
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

# Show which directories are actually included to the user
GET_PROPERTY(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
IF(VERBOSITY GREATER 4)
	FOREACH(dir ${dirs})
		MESSAGE(STATUS "Add to the include directories: '${dir}'")
	ENDFOREACH()
ENDIF()

IF(NOT (DEFINED SOFTDEVICE_DIR AND DEFINED SOFTDEVICE_DIR_HEX))
	SET(${SOFTDEVICE_DIR}, "${NRF51822_DIR}/components/softdevice/s${SOFTDEVICE_SERIES}/")
	SET(${SOFTDEVICE_DIR_HEX}, "hex")
ENDIF()

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

	ADD_CUSTOM_TARGET(generate-dat
		COMMAND echo
		COMMAND echo "*** Use srec_cat to create the INIT .dat file for the DFU"
		COMMAND echo
		COMMAND echo "srec_cat ${PROJECT_NAME}.hex -intel -exclude 0x3FC00 0x3FC20 -generate 0x3FC00 0x3FC04 -l-e-constant 0x01 4 -generate 0x3FC04 0x3FC08 -l-e-constant 0x00 4 -generate 0x3FC08 0x3FC0C -l-e-constant 0xFE 4 -generate 0x3FC0C 0x3FC20 -constant 0x00 -o ${PROJECT_NAME}.dat -intel"
		COMMAND srec_cat ${PROJECT_NAME}.hex -intel -exclude 0x3FC00 0x3FC20 -generate 0x3FC00 0x3FC04 -l-e-constant 0x01 4 -generate 0x3FC04 0x3FC08 -l-e-constant 0x00 4 -generate 0x3FC08 0x3FC0C -l-e-constant 0xFE 4 -generate 0x3FC0C 0x3FC20 -constant 0x00 -o ${PROJECT_NAME}.dat -intel
		DEPENDS ${PROJECT_NAME}.tmp
		COMMENT "Generate the INIT .dat file for the DFU"
		)

	ADD_CUSTOM_TARGET(combine
		COMMAND echo
		COMMAND echo "*** Use srec_cat to create one combined softdevice+app file"
		COMMAND echo
		COMMAND echo "srec_cat ${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_HEX}/${SOFTDEVICE}_softdevice.hex -intel bootloader.hex -intel ${PROJECT_NAME}.bin -binary -offset ${APPLICATION_START_ADDRESS} -o combined.hex -intel"
		COMMAND srec_cat ${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_HEX}/${SOFTDEVICE}_softdevice.hex -intel bootloader.hex -intel ${PROJECT_NAME}.bin -binary -offset ${APPLICATION_START_ADDRESS} -o combined.hex -intel
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

