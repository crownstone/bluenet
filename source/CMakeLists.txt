######################################################################################################################
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
SET(MAIN_FILE "cs_Crownstone.cpp")

# Start a project
PROJECT(${PROJECT_NAME}) 

#enable_language(CXX)
#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED on)
#set(CMAKE_CXX_EXTENSIONS OFF)

# The directory with some of the FindXXX modules
set(DEFAULT_CONF_CMAKE_PATH ${CMAKE_SOURCE_DIR}/conf/cmake)
set(DEFAULT_MODULES_PATH ${DEFAULT_CONF_CMAKE_PATH}/modules)
set(THIRD_MODULES_PATH ${DEFAULT_CONF_CMAKE_PATH}/modules/third)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CONF_CMAKE_PATH};${DEFAULT_MODULES_PATH};${THIRD_MODULES_PATH}")

MESSAGE(STATUS "Search for FindX files in ${CMAKE_MODULE_PATH}")

#include(${DEFAULT_MODULES_PATH}/load_configuration.cmake)
#load_configuration("${DEFAULT_CONFIGURATION_FILE}" CMAKE_ARGS)
#load_configuration("${BOARD_TARGET_CONFIGURATION_FILE}" CMAKE_ARGS)
#load_configuration("${BOARD_TARGET_OVERWRITE_CONFIGURATION_FILE}" CMAKE_ARGS)

include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)

#######################################################################################################################

if(NOT CONFIG_DIR)
  set(CONFIG_DIR "config")
endif()

if(NOT GDB_PORT)
  set(GDB_PORT "2331")
endif()

if(NOT BOARD_TARGET)
  set(BOARD_TARGET "2331")
endif()

# Get the following information via JlinkExe and then ShowEmuList
if(NOT SERIAL_NUM)
  set(SERIAL_NUM "")
endif()

#if(NOT GIT_BRANCH)
#  message(FATAL_ERROR "The code should live on a branch in a git repository")
#endif()

SET(VERBOSITY                                "4"           CACHE STRING "Verbosity")
SET(HOST_TARGET                              "ON"          CACHE BOOL "Compile for host platform")
#SET(COMPILATION_DAY                          "unset"       CACHE STRING "Day of compilation")
SET(NRF5_DIR                                 "unset"       CACHE STRING "The nRF5 sdk directory")
SET(WORKSPACE_DIR                            "unset"       CACHE STRING "The workspace directory")
SET(SOFTDEVICE_HEX_FILE                      ""            CACHE STRING "Softdevice hex file")
SET(PASS_FILE                                "dfu_pkg_signing_key" CACHE STRING "Pass file (containing key")
SET(KEY_FILE                                 "temp.key"    CACHE STRING "Pass file (containing key")
SET(SUPERUSER_SWITCH                         "sudo"        CACHE STRING "Switch to superuser")

MATH(EXPR SWO_PORT       "${GDB_PORT}+1")
MATH(EXPR TELNET_PORT    "${GDB_PORT}+2")

#######################################################################################################################
# Show variables
#######################################################################################################################

MESSAGE(STATUS "Set verbosity to level: ${VERBOSITY}")
#MESSAGE(STATUS "Git branch: ${GIT_BRANCH}")
#MESSAGE(STATUS "Day of compilation: ${COMPILATION_DAY}")
MESSAGE(STATUS "Compiler: ${CMAKE_C_COMPILER}")

IF(SERIAL_NUM STREQUAL "")
  MESSAGE(STATUS "Default JLink device (serial number not specified)")
ELSE()
  MESSAGE(STATUS "Serial number of JLink device to use ${SERIAL_NUM}")
ENDIF()

IF(GDB_PORT STREQUAL "2331")
	MESSAGE(STATUS "Default GDB port (2331)")
ENDIF()
MESSAGE(STATUS "GDB port to use ${GDB_PORT}")

#######################################################################################################################
# Include your own FindX cmake files and options in the following file!
#######################################################################################################################

#######################################################################################################################

MESSAGE(STATUS "Do not set application start address")
#MESSAGE(STATUS "Set application start address in linker script to ${APPLICATION_START_ADDRESS}")
#CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/conf/nRF-softdevice.ld.in ${CMAKE_SOURCE_DIR}/conf/nRF-softdevice.ld)

# Libraries (math, stdc++ libraries, etc.) 
# Note! We CANNOT use the size-constrained *_s versions! anymore
# We don't seem to use any libraries anymore... except for including everything that comes with -std=c+11
#SET(LIBS "-lc ${LIBRARY_RUNTIME} ${LIBRARY_MATH} -lstdc++")
SET(LIBS "")

# Some debug information
IF(VERBOSITY GREATER 4)
	MESSAGE(STATUS "\"${PROJECT_NAME}\" uses the following CXX flags: \"${CMAKE_CXX_FLAGS}\" (can be empty)")
	MESSAGE(STATUS "Linked libraries: ${LIBS}")
ENDIF()

# Source directory
SET(SOURCE_DIR "src")

SET(CMAKE_INCLUDE_SYSTEM_FLAG_C "-isystem ")
SET(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "-isystem ")

# Add separately include directories if we are to lazy to specify e.g. "#include <protocol/rbc_mesh.h>"
# We have removed include/third/nrf from here. This needs to be specified separately for the firmware versus the
# bootloader. Both need namely a different sdk_config.h file...
INCLUDE_DIRECTORIES("include")
INCLUDE_DIRECTORIES("include/ble")
INCLUDE_DIRECTORIES("include/third")

# TODO: Just only include th NRF5 directory and refer to the header files using that relative path in the source files
# Then we do not need to update the CMakeLists.txt file all the time
INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}")
INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/toolchain/cmsis/include")
INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/config")
INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/fstorage")
INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/experimental_section_vars")

SET(BOOTLOADER "_bootloader_.hex")

IF((DEFINED TEST_TARGET) AND "${TEST_TARGET}" STREQUAL "\"nrf5\"")
	MESSAGE(STATUS "Run test target. This will include source files as necessary")
ELSE()
	INCLUDE(crownstone.src)

	IF (NOT BUILD_MESHING)
		LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/timer/app_timer.c")
	ENDIF()

	LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/${MAIN_FILE}")

	IF(VERBOSITY GREATER 4)
		FOREACH(crownstone_file ${FOLDER_SOURCE})
			MESSAGE(STATUS "Include Crownstone file: " ${crownstone_file})
		ENDFOREACH()
	ENDIF()

	# We add the source files explicitly. This is recommended in the cmake system and will also force us all the time to
	# consider the size of the final binary. Do not include things, if not necessary!

	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/mdk/gcc_startup_nrf52.S")
	set_property(SOURCE "${NRF5_DIR}/modules/nrfx/mdk/gcc_startup_nrf52.S" PROPERTY LANGUAGE C)

	# The following files are only added for the logging module by Nordic. It might be good to remove these files to
	# save space in production. It should then be enclosed within a macro.
	# Those files are: nrf_strerror.c

	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/ble/common/ble_advdata.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/ble/common/ble_srv_common.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/atomic/nrf_atomic.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/atomic_fifo/nrf_atfifo.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/crc16/crc16.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/experimental_section_vars/nrf_section_iter.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/fds/fds.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/fstorage/nrf_fstorage.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/fstorage/nrf_fstorage_sd.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/hardfault/hardfault_implementation.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/hardfault/nrf52/handler/hardfault_handler_gcc.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/scheduler/app_scheduler.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/strerror/nrf_strerror.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/util/app_error.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/util/app_error_handler_gcc.c")
#	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/util/app_error_weak.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/util/app_util_platform.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/util/nrf_assert.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/softdevice/common/nrf_sdh.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/softdevice/common/nrf_sdh_ble.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/softdevice/common/nrf_sdh_soc.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/drivers/src/nrfx_comp.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/drivers/src/prs/nrfx_prs.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/hal/nrf_nvmc.c")
	# should be our own code, but SystemInit here contains a lot of PANs we don't have to solve subsequently...
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/mdk/system_nrf52.c")
	
	IF (CS_SERIAL_NRF_LOG_ENABLED)
		MESSAGE(STATUS "SERIAL from NORDIC enabled")
		LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/balloc/nrf_balloc.c")
		LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/log/src/nrf_log_backend_serial.c")
		LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/log/src/nrf_log_default_backends.c")
		LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/log/src/nrf_log_frontend.c")
		LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/log/src/nrf_log_str_formatter.c")
		LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/memobj/nrf_memobj.c")
		LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/ringbuf/nrf_ringbuf.c")
		LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/external/fprintf/nrf_fprintf.c")
		LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/external/fprintf/nrf_fprintf_format.c")
		IF (CS_SERIAL_NRF_LOG_ENABLED STREQUAL 1)
			LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/log/src/nrf_log_backend_rtt.c")
			LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/external/segger_rtt/SEGGER_RTT.c")
			LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/external/segger_rtt/SEGGER_RTT_printf.c")
		ENDIF()
		IF (CS_SERIAL_NRF_LOG_ENABLED STREQUAL 2)
			LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/log/src/nrf_log_backend_uart.c")
			LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/integration/nrfx/legacy/nrf_drv_uart.c")
			LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/drivers/src/nrfx_uart.c")
		ENDIF()
	ELSE()
		MESSAGE(STATUS "SERIAL from NORDIC disabled")
	ENDIF()

	IF (BUILD_MESHING)
		# All C files in mesh/core/src
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/nrf_mesh_utils.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/nrf_mesh_configure.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/queue.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/ccm_soft.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/flash_manager_defrag.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/rand.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/uri.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/timeslot.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/mesh_opt.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/nrf_mesh_keygen.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/bearer_event.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/enc.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/list.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/core_tx.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/timer.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/mesh_config_flashman_glue.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/msg_cache.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/net_beacon.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/cache.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/flash_manager_internal.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/network.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/core_tx_lpn.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/net_state.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/fifo.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/heartbeat.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/beacon.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/nrf_mesh.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/mesh_config.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/packet_buffer.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/mesh_config_backend.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/nrf_flash.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/transport.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/mesh_flash.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/msqueue.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/lpn.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/net_packet.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/nrf_mesh_opt.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/mesh_lpn_subman.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/event.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/toolchain.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/hal.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/flash_manager.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/timer_scheduler.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/aes.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/replay_cache.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/internal_event.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/fsm.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/timeslot_timer.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/aes_cmac.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/log.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/core_tx_local.c")

		# All C files in mesh/bearer/src
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/bearer_handler.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/ad_type_filter.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/rssi_filter.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/mesh_pa_lna.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/adv_packet_filter.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/scanner.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/gap_address_filter.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/radio_config.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/ad_listener.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/filter_engine.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/advertiser.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/broadcast.c")

		# All C files in mesh/prov/src except nrf_mesh_prov_bearer_gatt.c
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/prov/src/provisioning.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/prov/src/prov_provisionee.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/prov/src/prov_beacon.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/prov/src/prov_utils.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/prov/src/prov_bearer_adv.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/prov/src/nrf_mesh_prov.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/prov/src/prov_provisioner.c")

		# All C files in mesh/access/src
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/access/src/access_publish_retransmission.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/access/src/access.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/access/src/access_reliable.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/access/src/access_publish.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/access/src/device_state_manager.c")

		# All C files in mesh/dfu/src
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/dfu/src/nrf_mesh_dfu.c")

		# All C files in mesh/stack/src
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/stack/src/mesh_stack.c")

		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/models/foundation/config/src/config_server.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/models/foundation/config/src/composition_data.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/models/foundation/config/src/packed_index_list.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/models/foundation/health/src/health_server.c")

		# Any other mesh models that are used in your application
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/models/model_spec/common/src/model_common.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/models/model_spec/generic_onoff/src/generic_onoff_client.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/models/model_spec/generic_onoff/src/generic_onoff_server.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/examples/common/src/app_onoff.c")

		# external/micro-ecc/uECC.c
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/external/micro-ecc/uECC.c")

		# TODO: Do not use example files
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/examples/common/src/mesh_provisionee.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/examples/common/src/assertion_handler_weak.c")

		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/external/app_timer/app_timer_mesh.c")
		
		IF (MESH_MEM_BACKEND STREQUAL "stdlib")
			LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/mesh_mem_stdlib.c")
		ELSEIF (MESH_MEM_BACKEND STREQUAL "packet_mgr")
			LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/mesh_mem_packet_mgr.c")
			LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/packet_mgr.c")
		ELSEIF (MESH_MEM_BACKEND STREQUAL "mem_manager")
			LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/mesh_mem_mem_manager.c")
			LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/mem_manager/mem_manager.c")
		ELSE ()
			MESSAGE(FATAL_ERROR "Unknown mesh_mem_backend \"${MESH_MEM_BACKEND}\"")
		ENDIF()
		
		IF (EXPERIMENTAL_INSTABURST_ENABLED)
			LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/core_tx_instaburst.c")
			LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/adv_ext_packet.c")
			LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/adv_ext_tx.c")
			LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/instaburst.c")
			LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/instaburst_rx.c")
			LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/instaburst_tx.c")
		ELSE()
			LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/core_tx_adv.c")
		ENDIF()
	ENDIF()

	IF(VERBOSITY GREATER 4)
		FOREACH(nordic_file ${NORDIC_SOURCE})
			MESSAGE(STATUS "Include Nordic file: " ${nordic_file})
		ENDFOREACH()
	ENDIF()

	# We now have a variable FOLDER_SOURCE with all source files
	LIST(APPEND FOLDER_SOURCE ${NORDIC_SOURCE})

	# For Visual Studio group the files
	SOURCE_GROUP("Source files" FILES ${FOLDER_SOURCE})
	SOURCE_GROUP("Header files" FILES ${FOLDER_HEADER})

	#INCLUDE_DIRECTORIES(${INCLUDE_DIR})

	# Add to the -I flags
	# This is required for the Nordic files internally. They refer only to header files without component paths.
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/ble/ble_services")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/ble/common")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/device/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/drivers_nrf/common/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/drivers_nrf/comp/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/atomic/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/atomic_fifo/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/balloc/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/crc16/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/delay/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/experimental_log/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/experimental_log/src/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/experimental_memobj/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/experimental_section_vars/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/fds/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/fstorage/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/hardfault/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/log/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/log/src/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/memobj/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/ringbuf/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/scheduler/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/strerror/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/timer/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/trace/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/util/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/softdevice/common/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/softdevice/common/softdevice_handler/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/softdevice/s${SOFTDEVICE_SERIES}/headers")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/toolchain/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/toolchain/cmsis/include")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/toolchain/gcc/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/external/fprintf/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/integration/nrfx/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/modules/nrfx/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/modules/nrfx/drivers/include/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/modules/nrfx/hal/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/modules/nrfx/mdk/")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/external/segger_rtt")
	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/integration/nrfx/legacy/")

	INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/softdevice/s132/headers/nrf52")

	IF (BUILD_MESHING)
		INCLUDE_DIRECTORIES(SYSTEM "${MESH_SDK_DIR}/mesh/stack/api")
		INCLUDE_DIRECTORIES(SYSTEM "${MESH_SDK_DIR}/mesh/prov/api")
		INCLUDE_DIRECTORIES(SYSTEM "${MESH_SDK_DIR}/mesh/prov/include")
		INCLUDE_DIRECTORIES(SYSTEM "${MESH_SDK_DIR}/mesh/access/api")
		INCLUDE_DIRECTORIES(SYSTEM "${MESH_SDK_DIR}/mesh/access/include")
		INCLUDE_DIRECTORIES(SYSTEM "${MESH_SDK_DIR}/mesh/dfu/api")
		INCLUDE_DIRECTORIES(SYSTEM "${MESH_SDK_DIR}/mesh/dfu/include")
		INCLUDE_DIRECTORIES(SYSTEM "${MESH_SDK_DIR}/mesh/core/api")
		INCLUDE_DIRECTORIES(SYSTEM "${MESH_SDK_DIR}/mesh/core/include")
		INCLUDE_DIRECTORIES(SYSTEM "${MESH_SDK_DIR}/mesh/bearer/api")
		INCLUDE_DIRECTORIES(SYSTEM "${MESH_SDK_DIR}/mesh/bearer/include")
		INCLUDE_DIRECTORIES(SYSTEM "${MESH_SDK_DIR}/mesh/gatt/api")
		INCLUDE_DIRECTORIES(SYSTEM "${MESH_SDK_DIR}/mesh/gatt/include")
		INCLUDE_DIRECTORIES(SYSTEM "${MESH_SDK_DIR}/models/foundation/config/include")
		INCLUDE_DIRECTORIES(SYSTEM "${MESH_SDK_DIR}/models/foundation/health/include")
		INCLUDE_DIRECTORIES(SYSTEM "${MESH_SDK_DIR}/models/model_spec/common/include")
		INCLUDE_DIRECTORIES(SYSTEM "${MESH_SDK_DIR}/models/model_spec/generic_onoff/include")
		INCLUDE_DIRECTORIES(SYSTEM "${MESH_SDK_DIR}/external/micro-ecc")
		INCLUDE_DIRECTORIES(SYSTEM "${MESH_SDK_DIR}/examples/common/include")
		IF (MESH_MEM_BACKEND STREQUAL "mem_manager")
			INCLUDE_DIRECTORIES(SYSTEM "${NRF5_DIR}/components/libraries/mem_manager")
		ENDIF()
		
	ENDIF()

	IF(DEFINED SOFTDEVICE_DIR AND DEFINED SOFTDEVICE_DIR_API)
		INCLUDE_DIRECTORIES(SYSTEM "${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_API}")
	ENDIF()

ENDIF()
		
# Show which directories are actually included to the user
GET_PROPERTY(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
IF(VERBOSITY GREATER 4)
	FOREACH(dir ${dirs})
		MESSAGE(STATUS "Add to the include directories: '${dir}'")
	ENDFOREACH()
ENDIF()
		
IF(NOT (DEFINED SOFTDEVICE_DIR AND DEFINED SOFTDEVICE_DIR_HEX))
	SET(${SOFTDEVICE_DIR}, "${NRF5_DIR}/components/softdevice/s${SOFTDEVICE_SERIES}/")
	SET(${SOFTDEVICE_DIR_HEX}, "hex")
ENDIF()

IF((DEFINED TEST_TARGET) AND "${TEST_TARGET}" STREQUAL "\"nrf5\"")
	MESSAGE(STATUS "Add testing files")

	ENABLE_TESTING()

	#	ADD_SUBDIRECTORY(test/nrf5)

	INCLUDE(test/nrf5/CMakeLists.txt)
ELSE()

	# Set up our main executable.
	IF(FOLDER_SOURCE STREQUAL "")
		MESSAGE(FATAL_ERROR "No source code files found. Please add something")
	ELSE()

		# Get firmware and bootloader version
		include(${DEFAULT_MODULES_PATH}/get_versions.cmake)
		get_version_info("${WORKSPACE_DIR}/source" FIRMWARE_VERSION FIRMWARE_DFU_VERSION FIRMWARE_RC_VERSION)
		get_version_info("${WORKSPACE_DIR}/source/bootloader" BOOTLOADER_VERSION BOOTLOADER_DFU_VERSION BOOTLOADER_RC_VERSION)
		
		IF(SERIAL_NUM) 
			SET(SERIAL_NUM_SWITCH "--snr")
			SET(SERIAL_NUM_GDB_SELECT "-select")
			SET(SERIAL_NUM_GDB_PARAM "usb=${SERIAL_NUM}")
  			SET(SERIAL_NUM_DPARAM "-DSERIAL_NUM:STRING=${SERIAL_NUM}")
		ENDIF()

		MESSAGE(STATUS "Descend into bootloader directory and compile if needed")
		add_subdirectory(bootloader)
		
		IF(NOT EXISTS  ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.config)
			MESSAGE(FATAL_ERROR "There should be a CMakeBuild.config file in the ${CONFIG_DIR}/${BOARD_TARGET} directory")
		ENDIF()
		
		SET_SOURCE_FILES_PROPERTIES(${NORDIC_SOURCE}
			PROPERTIES COMPILE_FLAGS "-Wno-unused-function -Wno-unknown-pragmas"
		)

		SET_SOURCE_FILES_PROPERTIES(${FOLDER_SOURCE}
			PROPERTIES OBJECT_DEPENDS ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.config
		)
		IF(EXISTS  ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.overwrite.config)
			SET_SOURCE_FILES_PROPERTIES(${FOLDER_SOURCE}
				PROPERTIES OBJECT_DEPENDS ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.overwrite.config
			)
		ENDIF()

		message(STATUS "Configure cs_Git.cpp file")
		configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/cfg/cs_Git.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/src/cfg/cs_Git.cpp" @ONLY)
		list(APPEND GENERATED_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/src/cfg/cs_Git.cpp")
		
		ADD_EXECUTABLE(${PROJECT_NAME} ${FOLDER_SOURCE} ${GENERATED_SOURCES} ${FOLDER_HEADER} ${OBJECT_FILES} ${PROJECT_NAME}.bin ${PROJECT_NAME}.hex ${PROJECT_NAME}.elf ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.config)
		
		# Be more strict about our own files (then about Nordic's files)
		#SET(CROWNSTONE_ONLY_FLAGS "-W")
		SET(CROWNSTONE_ONLY_FLAGS "")
		SET_PROPERTY(SOURCE ${FOLDER_SOURCE} APPEND PROPERTY COMPILE_FLAGS ${CROWNSTONE_ONLY_FLAGS})

		target_include_directories(${PROJECT_NAME} PRIVATE "include/third/nrf")

		# This does lead to a cmake reconfigure when the file changes... However, nothing else gets rebuild after
		# the reconfiguration
		set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.config)
		IF(EXISTS  ${WORKSPACE_DIR}/${CONFIG_DIR}${BOARD_TARGET}/CMakeBuild.overwrite.config)
			set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.overwrite.config)
		ENDIF()
		
		set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${WORKSPACE_DIR}/source/VERSION)
		set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${WORKSPACE_DIR}/source/bootloader/VERSION)

		SET(INCLUDE_ONLY_SECTIONS "")

		#		ADD_CUSTOM_TARGET(get_git_info
		#	COMMAND ${CMAKE_COMMAND} -DDEFAULT_MODULES_PATH=${DEFAULT_MODULES_PATH} -DWORKSPACE_DIR=${WORKSPACE_DIR} -P ${DEFAULT_MODULES_PATH}/git_info.cmake
		#	COMMAND 
		#	COMMENT "Get git info for new build"
		#	)

		# https://stackoverflow.com/questions/5278444/adding-a-custom-command-with-the-file-name-as-a-target
		ADD_CUSTOM_COMMAND(OUTPUT ${PROJECT_NAME}.bin
			COMMAND ${CMAKE_OBJCOPY_OVERLOAD} ${INCLUDE_ONLY_SECTIONS} -O binary ${PROJECT_NAME} ${PROJECT_NAME}.bin
			COMMAND stat -t ${PROJECT_NAME}.bin | cut -f2 -d ' ' | xargs printf "** Firmware size : %s bytes"
			COMMAND ${CMAKE_COMMAND} -E echo ""
			COMMAND ${CMAKE_COMMAND} -E echo "** Available size: 360kB, that is 512kB - softdevice - bootloader - app storage"
			COMMAND ${CMAKE_COMMAND} -E echo "** Dual bank Size: 180kB = DFU_IMAGE_MAX_SIZE_BANKED"
			DEPENDS 
				${PROJECT_NAME}
			COMMENT "Object copy ${PROJECT_NAME} to ${PROJECT_NAME}.bin")
		
		ADD_CUSTOM_COMMAND(OUTPUT ${PROJECT_NAME}.hex
			COMMAND ${CMAKE_OBJCOPY_OVERLOAD} ${INCLUDE_ONLY_SECTIONS} -O ihex ${PROJECT_NAME} ${PROJECT_NAME}.hex
			DEPENDS 
				${PROJECT_NAME}
			COMMENT "Object copy ${PROJECT_NAME} to ${PROJECT_NAME}.hex")
		
		ADD_CUSTOM_COMMAND(OUTPUT ${PROJECT_NAME}.elf
			COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_NAME} ${PROJECT_NAME}.elf
			COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
			DEPENDS 
				${PROJECT_NAME}
			COMMENT "Copy ${PROJECT_NAME} to ${PROJECT_NAME}.elf")

		ADD_CUSTOM_TARGET(generate_dat
			COMMAND echo
			COMMAND echo "** Use srec_cat to create the INIT .dat file for the DFU"
			COMMAND echo
			COMMAND echo "srec_cat ${PROJECT_NAME}.hex -intel -exclude 0x3FC00 0x3FC20 -generate 0x3FC00 0x3FC04 -l-e-constant 0x01 4 -generate 0x3FC04 0x3FC08 -l-e-constant 0x00 4 -generate 0x3FC08 0x3FC0C -l-e-constant 0xFE 4 -generate 0x3FC0C 0x3FC20 -constant 0x00 -o ${PROJECT_NAME}.dat -intel"
			COMMAND srec_cat ${PROJECT_NAME}.hex -intel -exclude 0x3FC00 0x3FC20 -generate 0x3FC00 0x3FC04 -l-e-constant 0x01 4 -generate 0x3FC04 0x3FC08 -l-e-constant 0x00 4 -generate 0x3FC08 0x3FC0C -l-e-constant 0xFE 4 -generate 0x3FC0C 0x3FC20 -constant 0x00 -o ${PROJECT_NAME}.dat -intel
			DEPENDS ${PROJECT_NAME}.hex
			COMMENT "Generate the INIT .dat file for the DFU"
			)

		ADD_CUSTOM_TARGET(merge_all
			COMMAND echo
			COMMAND echo "** Use srec_cat to create one merged file"
			COMMAND echo "** Includes softdevice, bootloader, firmware, bootloader settings, and all UICR fields, except board version."
			COMMAND echo
			COMMAND echo "srec_cat ${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_HEX}/${SOFTDEVICE}_softdevice.hex -intel bootloader.hex -intel ${PROJECT_NAME}.hex -intel bootloader-settings.hex -intel -exclude 0x10001014 0x10001018 -generate 0x10001014 0x10001018 -l-e-constant $BOOTLOADER_START_ADDRESS 4 -exclude 0x10001018 0x1000101C -generate 0x10001018 0x1000101C -l-e-constant 0x0007E000 4 -o combined.hex -intel"
			COMMAND srec_cat ${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_HEX}/${SOFTDEVICE}_softdevice.hex -intel bootloader.hex -intel ${PROJECT_NAME}.hex -intel bootloader-settings.hex -intel -exclude 0x10001014 0x10001018 -generate 0x10001014 0x10001018 -l-e-constant $BOOTLOADER_START_ADDRESS 4 -exclude 0x10001018 0x1000101C -generate 0x10001018 0x1000101C -l-e-constant 0x0007E000 4 -o combined.hex -intel
			DEPENDS ${PROJECT_NAME}.bin
			COMMENT "Merge all but board version into one hex"
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
			DEPENDS ${PROJECT_NAME}.elf
			COMMENT "Read headers ${PROJECT_NAME}.elf"
			)
	
		# The gdb server can be started with configuration specific information
		ADD_CUSTOM_TARGET(debug_server
			COMMAND ${CMAKE_COMMAND} -E echo "** Debug server, keep running in separate shell"
			COMMAND JLinkGDBServer -Device nRF52832_xxAA ${SERIAL_NUM_GDB_SELECT} ${SERIAL_NUM_GDB_PARAM} -If SWD -speed 4000 -port ${GDB_PORT} -swoport ${SWO_PORT} -telnetport ${TELNET_PORT}
			COMMENT "Debug server, runs in background"
			)
		
		ADD_CUSTOM_TARGET(uart_client
			COMMAND ${CMAKE_COMMAND} -E echo "** UART client, keep running in separate shell"
			COMMAND ${SUPERUSER_SWITCH} minicom -b 230400 -c on -D /dev/ttyACM0
			COMMENT "UART client, runs in background"
			)
		
		ADD_CUSTOM_TARGET(rtt_client
			COMMAND ${CMAKE_COMMAND} -E echo "** RTT client, keep running in separate shell"
			COMMAND JLinkRTTClient
			COMMENT "RTT client, runs in background"
			)
		
		ADD_CUSTOM_TARGET(debug_application
			COMMAND ${CMAKE_COMMAND} -E echo "** Debug"
			COMMAND ${COMPILER_PATH}/bin/arm-none-eabi-gdb --command ${CMAKE_SOURCE_DIR}/conf/gdb/gdbinit --eval-command="target;remote;localhost:${GDB_PORT}" --command ${CMAKE_SOURCE_DIR}/conf/gdb/gdbstart --exec=${PROJECT_NAME}.elf ${PROJECT_NAME}.elf
			DEPENDS ${PROJECT_NAME}.elf
			COMMENT "Debug ${PROJECT_NAME}.elf"
			)
	
		SET(DEFAULT_NRFUTIL_DPARAM "-DDEFAULT_MODULES_PATH:STRING=${DEFAULT_MODULES_PATH}")
		SET(DEFAULT_NRFUTIL_READ_DPARAM "-DDEFAULT_MODULES_PATH:STRING=${DEFAULT_MODULES_PATH};-DINSTRUCTION:STRING=READ")

		ADD_CUSTOM_TARGET(write_application
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "** Use nrfjprog to write binary"
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND nrfjprog -f nrf52 --program ${PROJECT_NAME}.hex --sectorerase ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}
			DEPENDS ${PROJECT_NAME}.hex
			COMMENT "Flashes application to remote target board"
			)

		ADD_CUSTOM_TARGET(check
			COMMAND cppcheck --enable=style --quiet ${CMAKE_SOURCE_DIR} 
			COMMENT "Check for style or worse types of errors (linter)"
			)
		
		ADD_CUSTOM_TARGET(lint
			COMMAND cpplint --filter=-whitespace/tab --linelength=120 --recursive ${CMAKE_SOURCE_DIR} 
			COMMENT "Check for style according to Google C++ style guide"
			)
		
		ADD_CUSTOM_TARGET(generate_documentation
			COMMAND ${CMAKE_COMMAND} -E echo "Run doxygen from ${CMAKE_SOURCE_DIR}"
			COMMAND cd ${WORKSPACE_DIR} && doxygen source/doxygen.config
			COMMENT "Run documentation generator"
			)
		
		ADD_CUSTOM_TARGET(view_documentation
			COMMAND ${CMAKE_COMMAND} -E echo "Open an .html viewer through xdg-open"
			COMMAND xdg-open ${WORKSPACE_DIR}/docs/html/index.html
			COMMENT "View documentation"
			DEPENDS generate_documentation
			)
			
		# Create a new version, create new release directory
		# Copy CMakeBuild.config.relesae.default.release to this directory
		# Check fields (e.g. compare with CMakeBuild.config.default and see if fields are missing)
		# Check if we are on the master branch
		# Check if we are on the latest public commit
		# Check if we have non-committed changes
		# Check if git version does not exist
		# Create git commit with this release version
		ADD_CUSTOM_TARGET(create_git_release
			COMMAND ${CMAKE_COMMAND} -DDEFAULT_MODULES_PATH=${DEFAULT_MODULES_PATH} -DWORKSPACE_DIR=${WORKSPACE_DIR} -P ${DEFAULT_MODULES_PATH}/release.cmake
			COMMENT "Create git release"
			)

		MESSAGE(STATUS "Persistent flags disabled: ${PERSISTENT_FLAGS_DISABLED}")
		
		ADD_CUSTOM_TARGET(read_bootloader_address
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_NRFUTIL_READ_DPARAM} -DADDRESS:STRING=0x10001014 ${SERIAL_NUM_DPARAM} -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
			COMMENT "Read bootloader address"
			)
	
		# Actually piping over the command line might fail in another OS, this might be replaced by a particular
		# cmake utility function
		ADD_CUSTOM_TARGET(read_nrf_chip_version
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_NRFUTIL_READ_DPARAM} -DADDRESS:STRING=0x10000100 ${SERIAL_NUM_DPARAM} -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_NRFUTIL_READ_DPARAM} -DADDRESS:STRING=0x10000104 ${SERIAL_NUM_DPARAM} -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
			COMMENT "Read nRF chip version"
			)
		
		ADD_CUSTOM_TARGET(read_board_version
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_NRFUTIL_READ_DPARAM} -DADDRESS:STRING=${HARDWARE_BOARD_ADDRESS} ${SERIAL_NUM_DPARAM} -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
			COMMENT "Read board version"
			)

		# Ideally we only want to call load_hardware_version when the write_hw_version target is used
		#IF(NOT CONFIG_DIR STREQUAL "release")
			set(JLINK_LINUX_DOWNLOAD_URL_BOARD_HARDWARE_VERSION "")
			include(${DEFAULT_MODULES_PATH}/load_hardware_version_mapping.cmake)
			message(STATUS "Get version for hardware \"${HARDWARE_BOARD}\"")
			load_hardware_version(${CMAKE_SOURCE_DIR}/include/cfg/cs_Boards.h ${HARDWARE_BOARD} LOAD_BOARD_HARDWARE_VERSION)
			message(STATUS "Got ${LOAD_BOARD_HARDWARE_VERSION}")
			include(${DEFAULT_MODULES_PATH}/hex.cmake)
			to_hex_raw(${LOAD_BOARD_HARDWARE_VERSION} HARDWARE_BOARD_HEX 8 "0x")
			message(STATUS "Hexadecimal ${HARDWARE_BOARD_HEX}")
			#ENDIF()

		ADD_CUSTOM_TARGET(write_board_version
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "** Write board version ${LOAD_BOARD_HARDWARE_VERSION}"
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND nrfjprog -f nrf52 --memwr ${HARDWARE_BOARD_ADDRESS} --val ${LOAD_BOARD_HARDWARE_VERSION} ${SERIAL_NUM_SWITCH} ${SERIAL_NUM} 
			COMMENT "Flashes board version to remote target board"
			)
		
		ADD_CUSTOM_TARGET(build_bootloader_settings
			COMMAND ${CMAKE_COMMAND} -E echo "** Firmware version: ${FIRMWARE_VERSION}"
			COMMAND ${CMAKE_COMMAND} -E echo "** Bootloader version: ${BOOTLOADER_VERSION}"
			COMMAND ${CMAKE_COMMAND} -E echo "** Use files in directory: ${CMAKE_BINARY_DIR}"
			COMMAND nrfutil settings generate --family NRF52 --application "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex" --application-version ${FIRMWARE_DFU_VERSION} --bootloader-version ${BOOTLOADER_DFU_VERSION} --bl-settings-version 1 "${CMAKE_BINARY_DIR}/bootloader/bootloader_settings.hex"
			COMMAND ${CMAKE_COMMAND} -E echo "** Generated file: ${CMAKE_BINARY_DIR}/bootloader/bootloader_settings.hex"
			COMMENT "Create bootloader settings"
			)
		
		ADD_CUSTOM_TARGET(write_bootloader_settings
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "** Use nrfjprog to write bootloader settings ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}"
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND nrfjprog -f nrf52 --program ${CMAKE_BINARY_DIR}/bootloader/bootloader_settings.hex --sectorerase ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}
			COMMENT "Flashes bootloader settings to remote target board"
			DEPENDS build_bootloader_settings
			)
	
		ADD_CUSTOM_TARGET(merge_bootloader_settings
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "** Use mergehex to combine bootloader with bootloader settings"
			COMMAND ${CMAKE_COMMAND} -E echo "      Note that the settings can only be build with info from the application"
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND mergehex -m ${CMAKE_BINARY_DIR}/bootloader/bootloader.hex ${CMAKE_BINARY_DIR}/bootloader/bootloader_settings.hex -o ${CMAKE_BINARY_DIR}/bootloader/bootloader_with_settings.hex
			COMMENT "Merges bootloader with bootloader settings"
			)

		SET(FIRMWARE_DFU_ZIP     crownstone_all_${FIRMWARE_VERSION}.zip)
		SET(APPLICATION_DFU_ZIP  crownstone_${FIRMWARE_VERSION}.zip)
		SET(BOOTLOADER_DFU_ZIP   bootloader_${FIRMWARE_VERSION}.zip)

		ADD_CUSTOM_TARGET(generate_dfu_package_all
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "** Use nrfutil to generate DFU package"
			COMMAND ${CMAKE_COMMAND} -E echo "     Application: ${PROJECT_NAME}.hex"
			COMMAND ${CMAKE_COMMAND} -E echo "     Softdevice:  ${SOFTDEVICE_HEX_FILE}"
			COMMAND ${CMAKE_COMMAND} -E echo "     Bootloader:  bootloader.hex"
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "TODO: softdevice req/id needs to be extracted rather than hardcoded"
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "** Use pass to get encrypted key file, using entry: ${PASS_FILE}"
			COMMAND ${CMAKE_COMMAND} -E touch ${KEY_FILE}
			COMMAND pass ${PASS_FILE} > ${KEY_FILE}
			COMMAND ${CMAKE_COMMAND} -E echo "     Temp key file: ${KEY_FILE}"
			COMMAND nrfutil pkg generate --hw-version 52 --sd-req 0x00B7 --sd-id 0x00B7 --key-file ${KEY_FILE} --application ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex --application-version ${FIRMWARE_DFU_VERSION} --softdevice ${NRF5_DIR}/${SOFTDEVICE_HEX_FILE} --bootloader ${CMAKE_BINARY_DIR}/bootloader/bootloader.hex --bootloader-version ${BOOTLOADER_DFU_VERSION} all_dfu.zip
			COMMAND ${CMAKE_COMMAND} -E make_directory firmwares
			COMMAND ${CMAKE_COMMAND} -E remove ${KEY_FILE}
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMENT "Generate DFU package with Softdevice, bootloader, and application"
			)
		
		ADD_CUSTOM_TARGET(generate_dfu_package_application
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "** Use nrfutil to generate DFU package"
			COMMAND ${CMAKE_COMMAND} -E echo "     Application: ${PROJECT_NAME}.hex"
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "TODO: softdevice req needs to be extracted rather than hardcoded"
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "** Use pass to get encrypted key file, using entry: ${PASS_FILE}"
			COMMAND ${CMAKE_COMMAND} -E touch ${KEY_FILE}
			COMMAND pass ${PASS_FILE} > ${KEY_FILE}
			COMMAND ${CMAKE_COMMAND} -E echo "     Temp key file: ${KEY_FILE}"
			COMMAND echo nrfutil pkg generate --hw-version 52 --sd-req 0x00B7 --key-file ${KEY_FILE} --application ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex --application-version ${FIRMWARE_DFU_VERSION} application_dfu.zip
			COMMAND nrfutil pkg generate --hw-version 52 --sd-req 0x00B7 --key-file ${KEY_FILE} --application ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex --application-version ${FIRMWARE_DFU_VERSION} application_dfu.zip
			COMMAND ${CMAKE_COMMAND} -E make_directory firmwares
			COMMAND ${CMAKE_COMMAND} -E copy application_dfu.zip firmwares/crownstone_${BOOTLOADER_VERSION}.zip
			COMMAND ${CMAKE_COMMAND} -E remove ${KEY_FILE}
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMENT "Generate DFU package with application only"
			)

		ADD_CUSTOM_TARGET(generate_dfu_package_bootloader
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "** Use nrfutil to generate DFU package"
			COMMAND ${CMAKE_COMMAND} -E echo "     Bootloader:  bootloader.hex"
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "TODO: softdevice req needs to be extracted rather than hardcoded"
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "** Use pass to get encrypted key file, using entry: ${PASS_FILE}"
			COMMAND ${CMAKE_COMMAND} -E touch ${KEY_FILE}
			COMMAND pass ${PASS_FILE} > ${KEY_FILE}
			COMMAND ${CMAKE_COMMAND} -E echo "     Temp key file: ${KEY_FILE}"
			COMMAND nrfutil pkg generate --hw-version 52 --sd-req 0x00B7 --key-file ${KEY_FILE} --bootloader ${CMAKE_BINARY_DIR}/bootloader/bootloader.hex --bootloader-version ${BOOTLOADER_DFU_VERSION} bootloader_dfu.zip
			COMMAND ${CMAKE_COMMAND} -E make_directory bootloaders
			COMMAND ${CMAKE_COMMAND} -E copy bootloader_dfu.zip bootloaders/crownstone_${FIRMWARE_VERSION}.zip
			COMMAND ${CMAKE_COMMAND} -E remove ${KEY_FILE}
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMENT "Generate DFU package with bootloader only"
			)
		
	    # We can not use math(EXPR ...) because it does not have support for unsigned integers and older cmake versions 
		# do not support hexadecimal values either
		if(NOT ${PRODUCT_FAMILY} STREQUAL "" AND NOT ${PRODUCT_MARKET} STREQUAL "" AND NOT ${PRODUCT_TYPE} STREQUAL "")
			from_hex(${PRODUCT_FAMILY} PRODUCT_FAMILY_DEC)
			from_hex(${PRODUCT_MARKET} PRODUCT_MARKET_DEC)
			from_hex(${PRODUCT_TYPE} PRODUCT_TYPE_DEC)
			to_hex_raw(${PRODUCT_FAMILY_DEC} PRODUCT_FAMILY_HEX 2 "")
			to_hex_raw(${PRODUCT_MARKET_DEC} PRODUCT_MARKET_HEX 2 "")
			to_hex_raw(${PRODUCT_TYPE_DEC} PRODUCT_TYPE_HEX 2 "")
			set(PRODUCT_FAMILY_TYPE "${PRODUCT_FAMILY_HEX}${PRODUCT_MARKET_HEX}${PRODUCT_TYPE_HEX}")
			fill(${PRODUCT_FAMILY_TYPE} PRODUCT_FAMILY_TYPE 8 "F" "0x")
		endif()
		
		if(NOT ${PRODUCT_MAJOR} STREQUAL "" AND NOT ${PRODUCT_MINOR} STREQUAL "" AND NOT ${PRODUCT_PATCH} STREQUAL "")
			from_hex(${PRODUCT_MAJOR} PRODUCT_MAJOR_DEC)
			from_hex(${PRODUCT_MINOR} PRODUCT_MINOR_DEC)
			from_hex(${PRODUCT_PATCH} PRODUCT_PATCH_DEC)
			to_hex_raw(${PRODUCT_MAJOR_DEC} PRODUCT_MAJOR_HEX 2 "")
			to_hex_raw(${PRODUCT_MINOR_DEC} PRODUCT_MINOR_HEX 2 "")
			to_hex_raw(${PRODUCT_PATCH_DEC} PRODUCT_PATCH_HEX 2 "")
			set(MAJOR_MINOR_PATCH "${PRODUCT_MAJOR_HEX}${PRODUCT_MINOR_HEX}${PRODUCT_PATCH_HEX}")
			fill(${MAJOR_MINOR_PATCH} MAJOR_MINOR_PATCH 8 "F" "0x")
		endif()
		
		if(NOT ${PRODUCTION_YEAR} STREQUAL "" AND NOT ${PRODUCTION_WEEK} STREQUAL "" AND NOT ${PRODUCT_HOUSING} STREQUAL "")
			from_hex(${PRODUCT_HOUSING} PRODUCT_HOUSING_DEC)
			to_hex_raw(${PRODUCT_HOUSING_DEC} PRODUCT_HOUSING_HEX 2 "")

			fill(${PRODUCTION_YEAR} PRODUCTION_YEAR_DEC 2 "0" "")
			fill(${PRODUCTION_WEEK} PRODUCTION_WEEK_DEC 2 "0" "")

			set(PRODUCTION_RUN_HOUSING "${PRODUCTION_YEAR_DEC}${PRODUCTION_WEEK_DEC}${PRODUCT_HOUSING_HEX}")
			fill(${PRODUCTION_RUN_HOUSING} PRODUCTION_RUN_HOUSING 8 "F" "0x")
		endif()
	
		set(SREC_CAT_UICR_BOOTLOADER        "-exclude;0x10001014;0x10001018;-generate;0x10001014;0x10001018;-l-e-constant;${UICR_BOOTLOADER_ADDRESS};4")
		set(SREC_CAT_MBR_SETTINGS           "-exclude;0x10001018;0x1000101C;-generate;0x10001018;0x1000101C;-l-e-constant;${MBR_SETTINGS};4")
		set(SREC_CAT_HARDWARE_BOARD         "-exclude;0x10001084;0x10001088;-generate;0x10001084;0x10001088;-l-e-constant;${HARDWARE_BOARD_HEX};4")
		set(SREC_CAT_PRODUCT_FAMILY_TYPE    "-exclude;0x10001088;0x1000108C;-generate;0x10001088;0x1000108C;-l-e-constant;${PRODUCT_FAMILY_TYPE};4")
		set(SREC_CAT_MAJOR_MINOR_PATCH      "-exclude;0x1000108C;0x10001090;-generate;0x1000108C;0x10001090;-l-e-constant;${MAJOR_MINOR_PATCH};4")
		set(SREC_CAT_PRODUCTION_RUN_HOUSING "-exclude;0x10001090;0x10001094;-generate;0x10001090;0x10001094;-l-e-constant;${PRODUCTION_RUN_HOUSING};4")

		ADD_CUSTOM_TARGET(generate_factory_image
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "** Create factory images"
			COMMAND ${CMAKE_COMMAND} -E echo "     MBR_SETTINGS: ${MBR_SETTINGS}"
			COMMAND ${CMAKE_COMMAND} -E echo "     UICR_BOOTLOADER_ADDRESS: ${UICR_BOOTLOADER_ADDRESS}"
			COMMAND ${CMAKE_COMMAND} -E echo "     BOARD_HARDWARE_VERSION: ${HARDWARE_BOARD_HEX}"
			COMMAND ${CMAKE_COMMAND} -E echo "     PRODUCT_FAMILY_TYPE: ${PRODUCT_FAMILY_TYPE}"
			COMMAND ${CMAKE_COMMAND} -E echo "     MAJOR_MINOR_PATCH: ${MAJOR_MINOR_PATCH}"
			COMMAND ${CMAKE_COMMAND} -E echo "     PRODUCTION_RUN_HOUSING: ${PRODUCTION_RUN_HOUSING}"
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "** Image to be created: ${PROJECT_NAME}${IMAGE_FILENAME_POSTFIX}.hex"
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND test -z "${MBR_SETTINGS}" && echo "No MBR_SETTINGS set. Did you use the -DFACTORY_IMAGE option and are all fields set in the CMakeFactoryImage.config file?" && echo && exit 1 || exit 0
			COMMAND test -z "${UICR_BOOTLOADER_ADDRESS}" && echo "No UICR_BOOTLOADER_ADDRESS set. Did you use the -DFACTORY_IMAGE option and are all fields set in the CMakeFactoryImage.config file?" && echo && exit 1 || exit 0
			COMMAND test -z "${HARDWARE_BOARD_HEX}" && echo "No HARDWARE_BOARD_HEX set. Did you use the -DFACTORY_IMAGE option and are all fields set in the CMakeFactoryImage.config file?" && echo && exit 1 || exit 0
			COMMAND test -z "${PRODUCT_FAMILY_TYPE}" && echo "No PRODUCT_FAMILY_TYPE set. Did you use the -DFACTORY_IMAGE option and are all fields set in the CMakeFactoryImage.config file?" && echo && exit 1 || exit 0
			COMMAND test -z "${MAJOR_MINOR_PATCH}" && echo "No MAJOR_MINOR_PATCH set. Did you use the -DFACTORY_IMAGE option and are all fields set in the CMakeFactoryImage.config file?" && echo && exit 1 || exit 0
			COMMAND test -z "${PRODUCTION_RUN_HOUSING}" && echo "No PRODUCTION_RUN_HOUSING set. Did you use the -DFACTORY_IMAGE option and are all fields set in the CMakeFactoryImage.config file?" && echo && exit 1 || exit 0
			COMMAND ${CMAKE_COMMAND} -E echo "Use srec_cat to create a factory image"
			COMMAND ${CMAKE_COMMAND} -E make_directory factory-images
			COMMAND srec_cat ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex -intel ${SREC_CAT_UICR_BOOTLOADER} ${SREC_CAT_MBR_SETTINGS} ${SREC_CAT_HARDWARE_BOARD_HEX} ${SREC_CAT_PRODUCT_FAMILY_TYPE} ${SREC_CAT_MAJOR_MINOR_PATCH} ${SREC_CAT_PRODUCTION_RUN_HOUSING} -o factory-images/${PROJECT_NAME}${IMAGE_FILENAME_POSTFIX}.hex -intel
			COMMENT "Generate a factory image"
			)


		if(NOT EXISTS ${CMAKE_BINARY_DIR}/factory-images)
			message(STATUS "Create directory \"factory-images\"")
			file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/factory-images)
		endif()

		SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${PROJECT_NAME}.bin)
		SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${PROJECT_NAME}.elf)
	
		SET(FILE_MEMORY_LAYOUT "-Tgeneric_gcc_nrf52.ld")
		SET(PATH_FILE_MEMORY "-L${CMAKE_SOURCE_DIR}/include/third/nrf/")
		SET(PATH_FILE_MEMORY "${PATH_FILE_MEMORY} -L${NRF5_DIR}/modules/nrfx/mdk/")
		SET(CMAKE_EXE_LINKER_FLAGS "${PATH_FILE_MEMORY} ${FILE_MEMORY_LAYOUT} ${CMAKE_EXE_LINKER_FLAGS}")
	
		TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIBS})

		# We actually do not need the file without extension
		#INSTALL(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)

		# TARGETS are not accepted from other directoreis
		INSTALL(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin" DESTINATION .)
		INSTALL(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf" DESTINATION .)
		INSTALL(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex" DESTINATION .)
		INSTALL(DIRECTORY "${CMAKE_BINARY_DIR}/firmwares"       DESTINATION . FILES_MATCHING PATTERN "*.zip")
		INSTALL(DIRECTORY "${CMAKE_BINARY_DIR}/bootloaders"     DESTINATION . FILES_MATCHING PATTERN "*.zip")
		INSTALL(DIRECTORY "${CMAKE_BINARY_DIR}/factory-images"  DESTINATION . FILES_MATCHING PATTERN "*")
	ENDIF()

ENDIF()
