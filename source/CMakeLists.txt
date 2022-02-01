#######################################################################################################################
# The bluenet build systems uses CMake.
#
# Author:    Anne C. van Rossum
# Date:      Oct 28, 2013
# License:   Lesser General Public License (LGPL GNU v3), the Apache License, or the MIT license.
#
# Copyright © 2013 Crownstone
#######################################################################################################################

cmake_minimum_required(VERSION 3.10)

if(COMMAND cmake_policy)
	# only interpret arguments as variables when unquoted
	cmake_policy(SET CMP0054 NEW)
endif()

# Set application shorthand
set(APPLICATION_SHORTHAND "crownstone")

# Set name of project
set(PROJECT_NAME ${APPLICATION_SHORTHAND})

# Set the main file to use for this application
set(MAIN_FILE "cs_Crownstone.cpp")

# The directory with additional CMake files
set(DEFAULT_CONF_CMAKE_PATH ${CMAKE_SOURCE_DIR}/conf/cmake)

set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${DEFAULT_CONF_CMAKE_PATH}/override_cxx.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_C ${DEFAULT_CONF_CMAKE_PATH}/override_c.cmake)

# Start a project
project(${PROJECT_NAME})

# Other search directories for cmake
set(DEFAULT_MODULES_PATH ${DEFAULT_CONF_CMAKE_PATH}/modules)
set(THIRD_MODULES_PATH ${DEFAULT_CONF_CMAKE_PATH}/modules/third)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CONF_CMAKE_PATH};${DEFAULT_MODULES_PATH};${THIRD_MODULES_PATH}")

message(STATUS "Search for FindX files in ${CMAKE_MODULE_PATH}")

# Get compilation day, this will be used in the binary itself
string(TIMESTAMP COMPILATION_DAY "%Y-%m-%d")

# Get git info, this will be used in the binary in Debug build system
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)

message(STATUS "Git branch ${GIT_REFSPEC} at hash ${GIT_SHA1}")

#######################################################################################################################

if(NOT CONFIG_DIR)
	set(CONFIG_DIR "config")
endif()

if(NOT GDB_PORT)
	set(GDB_PORT "2331")
endif()

if(NOT BOARD_TARGET)
	set(BOARD_TARGET "default")
endif()

set(VERBOSITY                                "4"           CACHE STRING "Verbosity")
set(NRF5_DIR                                 "unset"       CACHE STRING "The nRF5 sdk directory")
set(WORKSPACE_DIR                            "unset"       CACHE STRING "The workspace directory")
set(SOFTDEVICE_HEX_FILE                      ""            CACHE STRING "Softdevice hex file")
set(PASS_FILE                                "crownstone/firmware/dfu_pkg_signing_key" CACHE STRING "Pass file (containing key")
set(KEY_FILE                                 "temp.key"    CACHE STRING "Pass file (containing key")
set(SUPERUSER_SWITCH                         "sudo"        CACHE STRING "Switch to superuser")

# Actually the deployment should not contain this information
math(EXPR SWO_PORT       "${GDB_PORT}+1")
math(EXPR TELNET_PORT    "${GDB_PORT}+2")
math(EXPR RTT_PORT       "${GDB_PORT}+16690")

#######################################################################################################################
# Show variables
#######################################################################################################################

message(STATUS "Hardware board: ${HARDWARE_BOARD}")

message(STATUS "Set verbosity to level: ${VERBOSITY}")
message(STATUS "Compiler: ${CMAKE_C_COMPILER}")

#######################################################################################################################
# Include your own FindX cmake files and options in the following file!
#######################################################################################################################

#######################################################################################################################

message(STATUS "Do not set application start address")
#message(STATUS "Set application start address in linker script to ${APPLICATION_START_ADDRESS}")
#CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/conf/nRF-softdevice.ld.in ${CMAKE_SOURCE_DIR}/conf/nRF-softdevice.ld)

# Libraries (math, stdc++ libraries, etc.) 
# Note! We CANNOT use the size-constrained *_s versions! anymore
# We don't seem to use any libraries anymore... except for including everything that comes with -std=c+11
#set(LIBS "-lc ${LIBRARY_RUNTIME} ${LIBRARY_MATH} -lstdc++")
set(LIBS "")

# Some debug information
if(VERBOSITY GREATER 4)
	message(STATUS "\"${PROJECT_NAME}\" uses the following CXX flags: \"${CMAKE_CXX_FLAGS}\" (can be empty)")
	message(STATUS "Linked libraries: ${LIBS}")
endif()

# Source directory
set(SOURCE_DIR "src")

# Get all includes from nordic
include(nordic.src)

set(BOOTLOADER "_bootloader_.hex")

if((DEFINED TEST_TARGET) AND "${TEST_TARGET}" STREQUAL "\"nrf5\"")
	message(STATUS "Run test target. This will include source files as necessary")
else()
	include(crownstone.src)

	if(NOT BUILD_MESHING)
		list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/timer/app_timer.c")
	endif()

	list(APPEND FOLDER_SOURCE "${SOURCE_DIR}/${MAIN_FILE}")

	if(VERBOSITY GREATER 4)
		foreach(crownstone_file ${FOLDER_SOURCE})
			message(STATUS "Include Crownstone file: " ${crownstone_file})
		endforeach()
	endif()

	# We add the source files explicitly. This is recommended in the cmake system and will also force us all the time to
	# consider the size of the final binary. Do not include things, if not necessary!

	if(DEVICE STREQUAL "nRF52832_xxAA")
		list(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/mdk/gcc_startup_nrf52.S")
		set_property(SOURCE "${NRF5_DIR}/modules/nrfx/mdk/gcc_startup_nrf52.S" PROPERTY LANGUAGE C)
	elseif(DEVICE STREQUAL "nRF52833")
		list(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/mdk/gcc_startup_nrf52833.S")
		set_property(SOURCE "${NRF5_DIR}/modules/nrfx/mdk/gcc_startup_nrf52833.S" PROPERTY LANGUAGE C)
	elseif(DEVICE STREQUAL "nRF52840_xxAA")
		list(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/mdk/gcc_startup_nrf52840.S")
		set_property(SOURCE "${NRF5_DIR}/modules/nrfx/mdk/gcc_startup_nrf52840.S" PROPERTY LANGUAGE C)
	else()
		message(FATAL_ERROR "Unkown device: ${DEVICE}")
	endif()



	# The following files are only added for the logging module by Nordic. It might be good to remove these files to
	# save space in production. It should then be enclosed within a macro.
	# Those files are: nrf_strerror.c

	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/ble/ble_db_discovery/ble_db_discovery.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/ble/common/ble_advdata.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/ble/common/ble_srv_common.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/atomic/nrf_atomic.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/atomic_fifo/nrf_atfifo.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/crc16/crc16.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/crc32/crc32.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/experimental_section_vars/nrf_section_iter.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/fds/fds.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/fstorage/nrf_fstorage.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/fstorage/nrf_fstorage_sd.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/hardfault/hardfault_implementation.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/hardfault/nrf52/handler/hardfault_handler_gcc.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/scheduler/app_scheduler.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/strerror/nrf_strerror.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/util/app_error.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/util/app_error_handler_gcc.c")
#	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/util/app_error_weak.c")


#	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/util/app_util_platform.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/util/nrf_assert.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/softdevice/common/nrf_sdh.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/softdevice/common/nrf_sdh_ble.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/softdevice/common/nrf_sdh_soc.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/drivers/src/nrfx_comp.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/drivers/src/nrfx_wdt.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/drivers/src/prs/nrfx_prs.c")
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/hal/nrf_nvmc.c")
	# should be our own code, but SystemInit here contains a lot of PANs we don't have to solve subsequently...
	list(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/mdk/system_nrf52.c")

	if(NORDIC_SDK_VERSION GREATER 15)
		list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/ble/nrf_ble_gq/nrf_ble_gq.c")
		list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/queue/nrf_queue.c")
	endif()

	if(CS_SERIAL_NRF_LOG_ENABLED)
		message(STATUS "SERIAL from NORDIC enabled")
		list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/balloc/nrf_balloc.c")
		list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/log/src/nrf_log_backend_serial.c")
		list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/log/src/nrf_log_default_backends.c")
		list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/log/src/nrf_log_frontend.c")
		list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/log/src/nrf_log_str_formatter.c")
		list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/memobj/nrf_memobj.c")
		list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/ringbuf/nrf_ringbuf.c")
		list(APPEND NORDIC_SOURCE "${NRF5_DIR}/external/fprintf/nrf_fprintf.c")
		list(APPEND NORDIC_SOURCE "${NRF5_DIR}/external/fprintf/nrf_fprintf_format.c")
		if(CS_SERIAL_NRF_LOG_ENABLED STREQUAL 1)
			list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/log/src/nrf_log_backend_rtt.c")
			list(APPEND NORDIC_SOURCE "${NRF5_DIR}/external/segger_rtt/SEGGER_RTT.c")
			list(APPEND NORDIC_SOURCE "${NRF5_DIR}/external/segger_rtt/SEGGER_RTT_printf.c")
		endif()
		if(CS_SERIAL_NRF_LOG_ENABLED STREQUAL 2)
			list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/log/src/nrf_log_backend_uart.c")
			list(APPEND NORDIC_SOURCE "${NRF5_DIR}/integration/nrfx/legacy/nrf_drv_uart.c")
			list(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/drivers/src/nrfx_uart.c")
		endif()
	else()
		if(NORDIC_SDK_VERSION GREATER 15)
			# Required now for gatt queue
			list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/memobj/nrf_memobj.c")
			list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/balloc/nrf_balloc.c")
		endif()
		message(STATUS "SERIAL from NORDIC disabled")
	endif()

	if(BUILD_MICROAPP_SUPPORT)
		list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/fstorage/nrf_fstorage.c")
		list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/fstorage/nrf_fstorage_sd.c")
	else()
		message(STATUS "Microapp support disabled, no fstorage module required")
	endif()

	if(BUILD_TWI)
		list(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/drivers/src/nrfx_twi.c")
	else()
		message(STATUS "Module for twi disabled")
	endif()
	
	if(BUILD_GPIOTE)
		list(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/drivers/src/nrfx_gpiote.c")
	else()
		message(STATUS "Module for gpio tasks and events disabled")
	endif()

	if(BUILD_MESHING)
		# All C files in mesh/core/src
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/nrf_mesh_utils.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/nrf_mesh_configure.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/queue.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/ccm_soft.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/flash_manager_defrag.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/rand.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/uri.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/timeslot.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/mesh_opt.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/nrf_mesh_keygen.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/bearer_event.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/enc.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/list.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/core_tx.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/timer.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/mesh_config_flashman_glue.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/msg_cache.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/net_beacon.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/cache.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/flash_manager_internal.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/network.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/core_tx_lpn.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/net_state.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/fifo.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/heartbeat.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/beacon.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/nrf_mesh.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/mesh_config.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/packet_buffer.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/mesh_config_backend.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/nrf_flash.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/transport.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/mesh_flash.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/msqueue.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/lpn.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/net_packet.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/nrf_mesh_opt.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/mesh_lpn_subman.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/event.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/toolchain.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/hal.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/flash_manager.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/timer_scheduler.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/aes.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/replay_cache.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/internal_event.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/fsm.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/timeslot_timer.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/aes_cmac.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/log.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/core_tx_local.c")

		# All C files in mesh/bearer/src
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/bearer_handler.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/ad_type_filter.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/rssi_filter.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/mesh_pa_lna.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/adv_packet_filter.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/scanner.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/gap_address_filter.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/radio_config.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/ad_listener.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/filter_engine.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/advertiser.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/broadcast.c")

		# All C files in mesh/prov/src except nrf_mesh_prov_bearer_gatt.c
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/prov/src/provisioning.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/prov/src/prov_provisionee.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/prov/src/prov_beacon.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/prov/src/prov_utils.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/prov/src/prov_bearer_adv.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/prov/src/nrf_mesh_prov.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/prov/src/prov_provisioner.c")

		# All C files in mesh/access/src
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/access/src/access_publish_retransmission.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/access/src/access.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/access/src/access_reliable.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/access/src/access_publish.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/access/src/device_state_manager.c")

		# All C files in mesh/dfu/src
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/dfu/src/nrf_mesh_dfu.c")

		# All C files in mesh/stack/src
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/stack/src/mesh_stack.c")

		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/models/foundation/config/src/config_server.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/models/foundation/config/src/composition_data.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/models/foundation/config/src/packed_index_list.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/models/foundation/health/src/health_server.c")

		# Any other mesh models that are used in your application
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/models/model_spec/common/src/model_common.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/models/model_spec/generic_onoff/src/generic_onoff_client.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/models/model_spec/generic_onoff/src/generic_onoff_server.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/examples/common/src/app_onoff.c")

		# external/micro-ecc/uECC.c
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/external/micro-ecc/uECC.c")

		# TODO: Do not use example files
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/examples/common/src/mesh_provisionee.c")
		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/examples/common/src/assertion_handler_weak.c")

		list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/external/app_timer/app_timer_mesh.c")
		if(MESH_SDK_VERSION_MAJOR EQUAL 5)
			list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/external/app_timer/app_timer_workaround.c")
		endif()
		
		if(MESH_MEM_BACKEND STREQUAL "stdlib")
			list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/mesh_mem_stdlib.c")
		elseif(MESH_MEM_BACKEND STREQUAL "packet_mgr")
			list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/mesh_mem_packet_mgr.c")
			list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/packet_mgr.c")
		elseif(MESH_MEM_BACKEND STREQUAL "mem_manager")
			list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/mesh_mem_mem_manager.c")
			list(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/mem_manager/mem_manager.c")
		else()
			message(FATAL_ERROR "Unknown mesh_mem_backend \"${MESH_MEM_BACKEND}\"")
		endif()
		
		if(EXPERIMENTAL_INSTABURST_ENABLED)
			list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/core_tx_instaburst.c")
			list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/adv_ext_packet.c")
			list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/adv_ext_tx.c")
			list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/instaburst.c")
			list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/instaburst_rx.c")
			list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bearer/src/instaburst_tx.c")
		else()
			list(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/core/src/core_tx_adv.c")
		endif()
	endif()

	if(VERBOSITY GREATER 4)
		foreach(nordic_file ${NORDIC_SOURCE})
			message(STATUS "Include Nordic file: " ${nordic_file})
		endforeach()
	endif()

	# We now have a variable FOLDER_SOURCE with all source files
	list(APPEND FOLDER_SOURCE ${NORDIC_SOURCE})

	# For Visual Studio group the files
	source_group("Source files" FILES ${FOLDER_SOURCE})
	source_group("Header files" FILES ${FOLDER_HEADER})

	if(DEFINED SOFTDEVICE_DIR AND DEFINED SOFTDEVICE_DIR_API)
		include_directories(SYSTEM "${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_API}")
	endif()

endif()

# Show which directories are actually included to the user
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY include_directories)
if(VERBOSITY GREATER 4)
	foreach(dir ${dirs})
		message(STATUS "Add to the include directories: '${dir}'")
	endforeach()
endif()

if(NOT(DEFINED SOFTDEVICE_DIR AND DEFINED SOFTDEVICE_DIR_HEX))
	set(${SOFTDEVICE_DIR}, "${NRF5_DIR}/components/softdevice/s${SOFTDEVICE_SERIES}/")
	set(${SOFTDEVICE_DIR_HEX}, "hex")
endif()

if((DEFINED TEST_TARGET) AND "${TEST_TARGET}" STREQUAL "\"nrf5\"")
	message(STATUS "Add testing files")

	enable_testing()

	include(test/nrf5/CMakeLists.txt)
else()

	# Set up our main executable.
	if(FOLDER_SOURCE STREQUAL "")
		message(FATAL_ERROR "No source code files found. Please add something")
	else()

		# Get firmware and bootloader version
		include(${DEFAULT_MODULES_PATH}/get_versions.cmake)
		get_version_info("${WORKSPACE_DIR}/source" FIRMWARE_VERSION FIRMWARE_DFU_VERSION FIRMWARE_RC_VERSION)
		get_version_info("${WORKSPACE_DIR}/source/bootloader" BOOTLOADER_VERSION BOOTLOADER_DFU_VERSION BOOTLOADER_RC_VERSION)

		# Create bootloader version as integers (DFU is already an integer, RC is treated separately, it's a string)
		string(REPLACE "-" ";" BOOTLOADER_WITHOUT_RC_LIST "${BOOTLOADER_VERSION}")
		list(GET BOOTLOADER_WITHOUT_RC_LIST 0 BOOTLOADER_VERSION_WITHOUT_RC)
		string(REPLACE "." ";" BOOTLOADER_VERSION_LIST "${BOOTLOADER_VERSION_WITHOUT_RC}")
		list(GET BOOTLOADER_VERSION_LIST 0 BOOTLOADER_VERSION_MAJOR)
		list(GET BOOTLOADER_VERSION_LIST 1 BOOTLOADER_VERSION_MINOR)
		list(GET BOOTLOADER_VERSION_LIST 2 BOOTLOADER_VERSION_PATCH)

		# Print so we can check correctness
		string(CONCAT PRINT_BOOTLOADER_VERSION "${BOOTLOADER_VERSION_MAJOR}."
			"${BOOTLOADER_VERSION_MINOR}."
			"${BOOTLOADER_VERSION_PATCH}"
			)
		if(BOOTLOADER_RC_VERSION)
			string(REGEX MATCHALL "[0-9]+" BOOTLOADER_VERSION_PRERELEASE "${BOOTLOADER_RC_VERSION}")
			string(CONCAT PRINT_BOOTLOADER_VERSION "${BOOTLOADER_RC_VERSION}-" "${BOOTLOADER_VERSION_PRERELEASE}")
		else()
			set(BOOTLOADER_VERSION_PRERELEASE 255)
		endif()
		message(STATUS "Bootloader version: ${PRINT_BOOTLOADER_VERSION}")

		# TODO: can be removed
		if(SERIAL_NUM)
			set(SERIAL_NUM_SWITCH "--snr")
			set(SERIAL_NUM_GDB_SELECT "-select")
			set(SERIAL_NUM_GDB_PARAM "usb=${SERIAL_NUM}")
		endif()

		message(STATUS "Descend into bootloader directory and compile if needed")
		add_subdirectory(bootloader)

		#message(STATUS "Descend into test directory and compile if needed")
		#add_subdirectory(test)

		set(DEFAULT_CONFIG_FILE "${CMAKE_SOURCE_DIR}/conf/cmake/CMakeBuild.config.default")
		set(TARGET_CONFIG_FILE "${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.config")
		set(TARGET_CONFIG_OVERWRITE_FILE "${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.overwrite.config")

		if(NOT EXISTS "${TARGET_CONFIG_FILE}")
			message(FATAL_ERROR "There should be a CMakeBuild.config file in the ${CONFIG_DIR}/${BOARD_TARGET} directory")
		endif()
		
		set_source_files_properties(${NORDIC_SOURCE}
			PROPERTIES COMPILE_FLAGS "-Wno-unused-function -Wno-unknown-pragmas"
		)

		set_source_files_properties(${FOLDER_SOURCE}
			PROPERTIES OBJECT_DEPENDS ${TARGET_CONFIG_FILE}
		)
		if(EXISTS "${TARGET_CONFIG_OVERWRITE_FILE}")
			set_source_files_properties(${FOLDER_SOURCE}
				PROPERTIES OBJECT_DEPENDS ${TARGET_CONFIG_OVERWRITE_FILE}
			)
		endif()

		message(STATUS "Configure cs_Git.cpp file")
		configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/cfg/cs_Git.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/src/cfg/cs_Git.cpp" @ONLY)
		list(APPEND GENERATED_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/src/cfg/cs_Git.cpp")
		
		message(STATUS "Configure cs_AutoConfig.cpp file")
		configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/cfg/cs_AutoConfig.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/src/cfg/cs_AutoConfig.cpp" @ONLY)
		list(APPEND GENERATED_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/src/cfg/cs_AutoConfig.cpp")
		
		message(STATUS "Configure cs_StaticConfig.h file")
		configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/cfg/cs_StaticConfig.h.in" "${CMAKE_CURRENT_BINARY_DIR}/include/cfg/cs_StaticConfig.h" @ONLY)

		message(STATUS "Configure cs_config.h file")
		configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/third/nrf/cs_config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/include/third/nrf/cs_config.h" @ONLY)

		message(STATUS "Configure nrf_symbols.ld file")
		configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/third/nrf/nrf_symbols.ld.in" "${CMAKE_CURRENT_BINARY_DIR}/include/third/nrf/nrf_symbols.ld" @ONLY)
		list(APPEND GENERATED_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/include/third/nrf/nrf_symbols.ld")

		link_directories(${CMAKE_CURRENT_BINARY_DIR}/include/third/nrf)
		link_directories(${CMAKE_CURRENT_BINARY_DIR}/include/third/nrf/sdk${NORDIC_SDK_VERSION_FULL})
	
		message(STATUS "Application start address: ${APPLICATION_START_ADDRESS}")
		message(STATUS "Application length: ${APPLICATION_LENGTH}")
		message(STATUS "RAM base: ${RAM_R1_BASE}")
		message(STATUS "RAM amount: ${RAM_APPLICATION_AMOUNT}")
		message(STATUS "RAM for IPC: ${RAM_BLUENET_IPC_LENGTH}")

		add_executable(${PROJECT_NAME} ${FOLDER_SOURCE} ${GENERATED_SOURCES} ${FOLDER_HEADER} ${OBJECT_FILES} ${PROJECT_NAME}.bin ${PROJECT_NAME}.hex ${PROJECT_NAME}.elf ${TARGET_CONFIG_FILE})
		
		# Be more strict about our own files (then about Nordic's files)
		#set(CROWNSTONE_ONLY_FLAGS "-W")
		set(CROWNSTONE_ONLY_FLAGS "")
		set_property(SOURCE ${FOLDER_SOURCE} APPEND PROPERTY COMPILE_FLAGS ${CROWNSTONE_ONLY_FLAGS})

		# Add path with files like sdk_config.h to include directories
		# The include/third/nrf file dir is required for files in the sdk (e.g. link "sdk_config.h" without path prefix)
		# The ${CMAKE_CURRENT_BINARY_DIR}/include dir is required for files generated through configure_file
		set(THIRD_PARTY_NRF_INCLUDE_PATH "include/third/nrf")
		message(STATUS "Include path: ${THIRD_PARTY_NRF_INCLUDE_PATH}")
		set(THIRD_PARTY_NRF_SDK_INCLUDE_PATH "include/third/nrf/sdk${NORDIC_SDK_VERSION_FULL}")
		message(STATUS "Include path: ${THIRD_PARTY_NRF_SDK_INCLUDE_PATH}")
		target_include_directories(${PROJECT_NAME} PRIVATE "${THIRD_PARTY_NRF_INCLUDE_PATH}" "${THIRD_PARTY_NRF_SDK_INCLUDE_PATH}" "${CMAKE_CURRENT_BINARY_DIR}/include/third/nrf" "${CMAKE_CURRENT_BINARY_DIR}/include")

		# Save intermediate files, these are used for binary logging.
		target_compile_options(${PROJECT_NAME} PRIVATE -save-temps=obj)

		# This does lead to a cmake reconfigure when the file changes... However, nothing else gets rebuild after
		# the reconfiguration
		set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${TARGET_CONFIG_FILE})
		if(EXISTS ${TARGET_CONFIG_OVERWRITE_FILE})
			set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${TARGET_CONFIG_OVERWRITE_FILE})
		endif()
		
		set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${WORKSPACE_DIR}/source/VERSION)
		set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${WORKSPACE_DIR}/source/bootloader/VERSION)

		# Reconfigure cmake to get up-to-date GIT_SHA1
		set_property(GLOBAL APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/.git/index")
		
		# Make also depend on helper files
		set_property(GLOBAL APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/conf/cmake/crownstone.defs.cmake")
		set_property(GLOBAL APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/conf/cmake/crownstone.src.cmake")

		set(INCLUDE_ONLY_SECTIONS "")

		# https://stackoverflow.com/questions/5278444/adding-a-custom-command-with-the-file-name-as-a-target
		add_custom_command(OUTPUT ${PROJECT_NAME}.bin
			COMMAND ${CMAKE_OBJCOPY_OVERLOAD} ${INCLUDE_ONLY_SECTIONS} -O binary ${PROJECT_NAME} ${PROJECT_NAME}.bin
			COMMAND stat -t ${PROJECT_NAME}.bin | cut -f2 -d ' ' | xargs printf "** Firmware size : %s bytes"
			COMMAND ${CMAKE_COMMAND} -E echo ""
			COMMAND ${CMAKE_COMMAND} -E echo "** Available size: 360kB, that is 512kB - softdevice - bootloader - app storage"
			COMMAND ${CMAKE_COMMAND} -E echo "** Dual bank Size: 180kB = DFU_IMAGE_MAX_SIZE_BANKED"
			DEPENDS
				${PROJECT_NAME}
			COMMENT "Object copy ${PROJECT_NAME} to ${PROJECT_NAME}.bin")
		
		add_custom_command(OUTPUT ${PROJECT_NAME}.hex
			COMMAND ${CMAKE_OBJCOPY_OVERLOAD} ${INCLUDE_ONLY_SECTIONS} -O ihex ${PROJECT_NAME} ${PROJECT_NAME}.hex
			DEPENDS
				${PROJECT_NAME}
			COMMENT "Object copy ${PROJECT_NAME} to ${PROJECT_NAME}.hex")
		
		add_custom_command(OUTPUT ${PROJECT_NAME}.elf
			COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_NAME} ${PROJECT_NAME}.elf
			COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
			COMMAND ${CMAKE_COMMAND} -DDEFAULT_MODULES_PATH=${DEFAULT_MODULES_PATH} -DCMAKE_SIZE=${CMAKE_SIZE} -DBINARY=${PROJECT_NAME}.elf -DOFFSET=${APPLICATION_START_ADDRESS} -DMAX_SIZE=${FLASH_MICROAPP_BASE} -P ${DEFAULT_MODULES_PATH}/check_sizes.cmake
			DEPENDS
				${PROJECT_NAME}
			COMMENT "Copy ${PROJECT_NAME} to ${PROJECT_NAME}.elf")

		add_custom_target(generate_dat
			COMMAND echo
			COMMAND echo "** Use srec_cat to create the INIT .dat file for the DFU"
			COMMAND echo
			COMMAND echo "srec_cat ${PROJECT_NAME}.hex -intel -exclude 0x3FC00 0x3FC20 -generate 0x3FC00 0x3FC04 -l-e-constant 0x01 4 -generate 0x3FC04 0x3FC08 -l-e-constant 0x00 4 -generate 0x3FC08 0x3FC0C -l-e-constant 0xFE 4 -generate 0x3FC0C 0x3FC20 -constant 0x00 -o ${PROJECT_NAME}.dat -intel"
			COMMAND srec_cat ${PROJECT_NAME}.hex -intel -exclude 0x3FC00 0x3FC20 -generate 0x3FC00 0x3FC04 -l-e-constant 0x01 4 -generate 0x3FC04 0x3FC08 -l-e-constant 0x00 4 -generate 0x3FC08 0x3FC0C -l-e-constant 0xFE 4 -generate 0x3FC0C 0x3FC20 -constant 0x00 -o ${PROJECT_NAME}.dat -intel
			DEPENDS ${PROJECT_NAME}.hex
			COMMENT "Generate the INIT .dat file for the DFU"
			)

		add_custom_target(merge_all
			COMMAND echo
			COMMAND echo "** Use srec_cat to create one merged file"
			COMMAND echo "** Includes softdevice, bootloader, firmware, bootloader settings, and all UICR fields, except board version."
			COMMAND echo
			COMMAND echo "srec_cat ${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_HEX}/${SOFTDEVICE}_softdevice.hex -intel bootloader.hex -intel ${PROJECT_NAME}.hex -intel bootloader-settings.hex -intel -exclude 0x10001014 0x10001018 -generate 0x10001014 0x10001018 -l-e-constant $BOOTLOADER_START_ADDRESS 4 -exclude 0x10001018 0x1000101C -generate 0x10001018 0x1000101C -l-e-constant 0x0007E000 4 -o combined.hex -intel"
			COMMAND srec_cat ${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_HEX}/${SOFTDEVICE}_softdevice.hex -intel bootloader.hex -intel ${PROJECT_NAME}.hex -intel bootloader-settings.hex -intel -exclude 0x10001014 0x10001018 -generate 0x10001014 0x10001018 -l-e-constant $BOOTLOADER_START_ADDRESS 4 -exclude 0x10001018 0x1000101C -generate 0x10001018 0x1000101C -l-e-constant 0x0007E000 4 -o combined.hex -intel
			DEPENDS ${PROJECT_NAME}.bin
			COMMENT "Merge all but board version into one hex"
			)

		add_custom_target(analyze
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
	
		# TODO: Rename CONFIG_FILE here to TARGET_RUNTIME_CONFIG_FILE
		set(DEFAULT_TOOL_PARAM "-DDEFAULT_MODULES_PATH:STRING=${DEFAULT_MODULES_PATH}\
			-DDEFAULT_CONFIG_FILE:STRING=${DEFAULT_CONFIG_FILE}\
			-DTARGET_CONFIG_FILE:STRING=${TARGET_CONFIG_FILE}\
			-DTARGET_CONFIG_OVERWRITE_FILE:STRING=${TARGET_CONFIG_OVERWRITE_FILE}\
			-DCONFIG_FILE:STRING=${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.runtime.config\
			-DKEYS_JSON_FILE:STRING=${KEYS_JSON_FILE}\
			-DDEVICE:STRING=${DEVICE}\
			-DNRF_DEVICE_FAMILY:STRING=${NRF_DEVICE_FAMILY}\
			-DSERIAL_NUM:STRING=${SERIAL_NUM}\
			-DGDB_PORT:STRING=${GDB_PORT}\
			-DSWO_PORT:STRING=${SWO_PORT}\
			-DTELNET_PORT:STRING=${TELNET_PORT}\
			-DUART_DEVICE:STRING=${UART_DEVICE}\
			-DUART_BAUDRATE:STRING=${UART_BAUDRATE}\
			-DRTT_PORT:STRING=${RTT_PORT}")
		
		# The gdb server can be started with configuration specific information
		add_custom_target(debug_server
			COMMAND ${CMAKE_COMMAND} -E echo "** Debug server, keep running in separate shell"
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=START_GDB_SERVER" -P ${DEFAULT_MODULES_PATH}/debug_tool.cmake
			COMMENT "Debug server, runs in background"
			)

		add_custom_target(uart_client
			COMMAND ${CMAKE_COMMAND} -E echo "** UART client, keep running in separate shell"
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=START_MINICOM" -P ${DEFAULT_MODULES_PATH}/uart_tool.cmake
			COMMENT "UART client, runs in background"
			)
		
		set(UART_CONFIG_PARAM "-DCONFIG_NAME:STRING=UART_DEVICE")

		add_custom_target(uart_binary_client
			COMMAND ${CMAKE_COMMAND} -E echo "** UART client, keep running in separate shell"
			COMMAND bash -c "cmake ${DEFAULT_TOOL_PARAM} ${UART_CONFIG_PARAM} -P ${DEFAULT_MODULES_PATH}/get_config.cmake 2>&1 | xargs -I uart ${WORKSPACE_DIR}/scripts/log-client.py -d uart -l ${CMAKE_CURRENT_BINARY_DIR}/extracted_logs.json"
			COMMENT "UART client, runs in background"
			VERBATIM
			)
		
		add_custom_target(rtt_client
			COMMAND ${CMAKE_COMMAND} -E echo "** RTT client, keep running in separate shell"
			COMMAND JLinkRTTClient -RTTTelnetPort ${RTT_PORT}
			COMMENT "RTT client, runs in background"
			)
		
		add_custom_target(extract_logs
			COMMAND ${CMAKE_COMMAND} -E echo "** Extract logs"
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=EXTRACT" -P ${DEFAULT_MODULES_PATH}/extract_logs.cmake
			COMMENT "Extract logs from preprocessed source files."
			)
		
		add_custom_target(debug_application
			COMMAND ${CMAKE_COMMAND} -E echo "** Debug"
			COMMAND ${COMPILER_PATH}/bin/arm-none-eabi-gdb-py --command ${CMAKE_SOURCE_DIR}/conf/gdb/gdbinit --eval-command="target;remote;localhost:${GDB_PORT}" --command ${CMAKE_SOURCE_DIR}/conf/gdb/gdbstart --exec=${PROJECT_NAME}.elf ${PROJECT_NAME}.elf
			DEPENDS ${PROJECT_NAME}.elf
			COMMENT "Debug ${PROJECT_NAME}.elf"
			)
		
		add_custom_target(debug_bootloader
			COMMAND ${CMAKE_COMMAND} -E echo "** Debug bootloader"
			COMMAND ${COMPILER_PATH}/bin/arm-none-eabi-gdb-py --command ${CMAKE_SOURCE_DIR}/conf/gdb/gdbinit --eval-command="target;remote;localhost:${GDB_PORT}" --command ${CMAKE_SOURCE_DIR}/conf/gdb/gdbstart --exec=bootloader.elf bootloader.elf
			WORKING_DIRECTORY bootloader
			DEPENDS bootloader
			COMMENT "Debug bootloader.elf"
			)

		add_custom_target(write_application
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=WRITE_BINARY" "-DBINARY=${PROJECT_NAME}.hex" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
			DEPENDS ${PROJECT_NAME}.hex
			COMMENT "Flashes application to remote target board (takes a while)"
			)

		add_custom_target(setup
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=SETUP" -P ${DEFAULT_MODULES_PATH}/cstool.cmake
			COMMENT "Setup Crownstone via csutil"
			)

		add_custom_target(factory_reset
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=FACTORY_RESET" -P ${DEFAULT_MODULES_PATH}/cstool.cmake
			COMMENT "Perform a factory reset of a Crownstone via csutil"
			)

		add_custom_target(memory_usage_test_client
			COMMAND ${WORKSPACE_DIR}/scripts/mem-usage-test.py
			COMMENT "Run memory usage test client, which communicates with a Crownstone."
			)

		add_custom_target(check
			COMMAND cppcheck --enable=style --quiet ${CMAKE_SOURCE_DIR} 
			COMMENT "Check for style or worse types of errors (linter)"
			)
		
		add_custom_target(lint
			COMMAND cpplint --filter=-whitespace/tab --linelength=120 --recursive ${CMAKE_SOURCE_DIR} 
			COMMENT "Check for style according to Google C++ style guide"
			)
		
		add_custom_target(generate_documentation
			COMMAND ${CMAKE_COMMAND} -E echo "Run doxygen from ${CMAKE_SOURCE_DIR}"
			COMMAND cd ${WORKSPACE_DIR} && doxygen source/doxygen.config
			COMMENT "Run documentation generator"
			)
		
		add_custom_target(view_documentation
			COMMAND ${CMAKE_COMMAND} -E echo "Open an .html viewer through xdg-open"
			COMMAND xdg-open ${WORKSPACE_DIR}/docs/html/index.html
			COMMENT "View documentation"
			DEPENDS generate_documentation
			)
		
		add_custom_target(list_debuggers
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=LIST" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
			COMMENT "List debuggers"
			)

		add_custom_target(erase
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=ERASE" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
			COMMENT "Erase everything on target board"
			)

		add_custom_target(reset
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=RESET" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
			COMMENT "Reset target board"
			)
			
		message(STATUS "Persistent flags disabled: ${PERSISTENT_FLAGS_DISABLED}")
		
		add_custom_target(read_bootloader_address
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=READ" "-DADDRESS=0x10001014" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
			COMMENT "Read bootloader address"
			)
		
		add_custom_target(read_mbr_parameter_page_address
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=READ" "-DADDRESS=0x10001018" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
			COMMENT "Read MBR parameter page address"
			)
	
		# Actually piping over the command line might fail in another OS, this might be replaced by a particular
		# cmake utility function
		add_custom_target(read_nrf_chip_version
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=READ" "-DADDRESS=0x10000100" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=READ" "-DADDRESS=0x10000104" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
			COMMENT "Read nRF chip version"
			)
		
		add_custom_target(read_board_version
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=READ" "-DADDRESS:STRING=${HARDWARE_BOARD_ADDRESS}" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
			COMMENT "Read board version"
			)

		add_custom_target(read_mac_address
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=READ" "-DADDRESS=0x100000A4" "-DCOUNT=8" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
			COMMENT "Read MAC address"
			)

		add_custom_target(write_config
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} -P ${DEFAULT_MODULES_PATH}/writeconfig.cmake
			COMMMENT "Write config"
			)

		# Ideally we only want to call load_hardware_version when the write_hw_version target is used
		#if(NOT CONFIG_DIR STREQUAL "release")

		set(JLINK_LINUX_DOWNLOAD_URL_BOARD_HARDWARE_VERSION "")
		include(${DEFAULT_MODULES_PATH}/load_hardware_version_mapping.cmake)
		message(STATUS "Get version for hardware \"${HARDWARE_BOARD}\"")
		load_hardware_version(${CMAKE_SOURCE_DIR}/include/cfg/cs_Boards.h ${HARDWARE_BOARD} LOAD_BOARD_HARDWARE_VERSION)
		message(STATUS "Got ${LOAD_BOARD_HARDWARE_VERSION}")
		include(${DEFAULT_MODULES_PATH}/hex.cmake)
		to_hex_raw(${LOAD_BOARD_HARDWARE_VERSION} HARDWARE_BOARD_HEX 8 "0x")
		message(STATUS "Hexadecimal ${HARDWARE_BOARD_HEX}")
		#endif()

		add_custom_target(write_board_version
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=WRITE" "-DADDRESS=${HARDWARE_BOARD_ADDRESS}" "-DVALUE=${LOAD_BOARD_HARDWARE_VERSION}" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
			COMMENT "Flashes board version to remote target board"
			)
	
		add_custom_target(increment_application_version
			COMMAND bash -c "perl -i -pe '$. == 2 && s/(\\d+)/$1+1/eg' ${CMAKE_SOURCE_DIR}/VERSION" VERBATIM
			COMMENT "Increment application version (in VERSION file)"
			)
			
		set(BOOTLOADER_HEX_FILE "${CMAKE_BINARY_DIR}/bootloader/bootloader_settings.hex")
		set(BOOTLOADER_BIN_FILE "${CMAKE_BINARY_DIR}/bootloader/bootloader_settings.bin")

		add_custom_target(build_bootloader_settings
			COMMAND ${CMAKE_COMMAND} -E echo "** Firmware version: ${FIRMWARE_VERSION}"
			COMMAND ${CMAKE_COMMAND} -E echo "** Bootloader version: ${BOOTLOADER_VERSION}"
			COMMAND ${CMAKE_COMMAND} -E echo "** Use files in directory: ${CMAKE_BINARY_DIR}"
			COMMAND ${CMAKE_COMMAND} -E echo "nrfutil settings generate --family NRF52 --application "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex" --application-version ${FIRMWARE_DFU_VERSION} --bootloader-version ${BOOTLOADER_DFU_VERSION} --bl-settings-version 2 ${BOOTLOADER_HEX_FILE}"
			COMMAND nrfutil settings generate --family NRF52 --application "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex" --application-version ${FIRMWARE_DFU_VERSION} --bootloader-version ${BOOTLOADER_DFU_VERSION} --bl-settings-version 2 "${BOOTLOADER_HEX_FILE}" 
			COMMAND ${CMAKE_OBJCOPY_OVERLOAD} -I ihex ${BOOTLOADER_HEX_FILE} -O binary ${BOOTLOADER_BIN_FILE}
			COMMAND ${CMAKE_COMMAND} -E echo "** Generated file: ${BOOTLOADER_HEX_FILE}"
			COMMENT "Create bootloader settings"
			)
		
		add_custom_target(write_bootloader_settings
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=WRITE_BINARY" "-DBINARY=${BOOTLOADER_HEX_FILE}" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
			COMMENT "Flashes bootloader settings to remote target board"
			DEPENDS build_bootloader_settings
			)
		
		add_custom_target(read_bootloader_settings
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=READ" "-DADDRESS=0x7F00" "-DCOUNT=128" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
			COMMENT "Read bootloader settings from remote target board"
			)
		
		add_custom_target(read_bootloader_settings_backup
			COMMAND ${CMAKE_COMMAND} ${DEFAULT_TOOL_PARAM} "-DINSTRUCTION=READ" "-DADDRESS=0x7E00" "-DCOUNT=128" -P ${DEFAULT_MODULES_PATH}/nrfjprog.cmake
			COMMENT "Read bootloader settings from remote target board"
			)
	
		add_custom_target(merge_bootloader_settings
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "** Use mergehex to combine bootloader with bootloader settings"
			COMMAND ${CMAKE_COMMAND} -E echo "      Note that the settings can only be build with info from the application"
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND mergehex -m ${CMAKE_BINARY_DIR}/bootloader/bootloader.hex ${CMAKE_BINARY_DIR}/bootloader/bootloader_settings.hex -o ${CMAKE_BINARY_DIR}/bootloader/bootloader_with_settings.hex
			COMMENT "Merges bootloader with bootloader settings"
			)

		set(FIRMWARE_DFU_ZIP     crownstone_all_${FIRMWARE_VERSION}.zip)
		set(APPLICATION_DFU_ZIP  crownstone_${FIRMWARE_VERSION}.zip)
		set(BOOTLOADER_DFU_ZIP   bootloader_${BOOTLOADER_VERSION}.zip)

		add_custom_target(extract_sd_id
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "** Extract id from softdevice file: ${SOFTDEVICE_HEX_FILE}"
			COMMAND ${CMAKE_OBJCOPY_OVERLOAD} -I ihex ${NRF5_DIR}/${SOFTDEVICE_HEX_FILE} -O binary check.bin
			COMMAND bash -c "xxd -s 0x300C -l 2 -e -u check.bin | awk '{print \"0x\"$2}' > softdevice_id" VERBATIM
			COMMAND bash -c "echo \"   Softdevice id: $(cat softdevice_id)\"" VERBATIM
			COMMAND ${CMAKE_COMMAND} -E echo
		)

		add_custom_target(generate_dfu_package_all
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "** Use nrfutil to generate DFU package"
			COMMAND ${CMAKE_COMMAND} -E echo "     Application: ${PROJECT_NAME}.hex"
			COMMAND ${CMAKE_COMMAND} -E echo "     Softdevice:  ${SOFTDEVICE_HEX_FILE}"
			COMMAND ${CMAKE_COMMAND} -E echo "     Bootloader:  bootloader.hex"
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND bash -c "echo \"Softdevice id: $(cat softdevice_id)\"" VERBATIM
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "** Use pass to get encrypted key file, using entry: ${PASS_FILE}"
			COMMAND ${CMAKE_COMMAND} -E touch ${KEY_FILE}
			COMMAND pass ${PASS_FILE} > ${KEY_FILE} USES_TERMINAL
			COMMAND ${CMAKE_COMMAND} -E echo "     Temp key file: ${KEY_FILE}"
			COMMAND bash -c "echo 'nrfutil pkg generate --hw-version 52 --sd-req $(cat softdevice_id) --sd-id $(cat softdevice_id) --key-file ${KEY_FILE} --application ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex --application-version ${FIRMWARE_DFU_VERSION} --softdevice ${NRF5_DIR}/${SOFTDEVICE_HEX_FILE} --bootloader ${CMAKE_BINARY_DIR}/bootloader/bootloader.hex --bootloader-version ${BOOTLOADER_DFU_VERSION} all_dfu.zip'" VERBATIM USES_TERMINAL
			COMMAND bash -c "nrfutil pkg generate --hw-version 52 --sd-req $(cat softdevice_id) --sd-id $(cat softdevice_id) --key-file ${KEY_FILE} --application ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex --application-version ${FIRMWARE_DFU_VERSION} --softdevice ${NRF5_DIR}/${SOFTDEVICE_HEX_FILE} --bootloader ${CMAKE_BINARY_DIR}/bootloader/bootloader.hex --bootloader-version ${BOOTLOADER_DFU_VERSION} all_dfu.zip" VERBATIM USES_TERMINAL
			COMMAND ${CMAKE_COMMAND} -E make_directory dfu
			COMMAND ${CMAKE_COMMAND} -E copy all_dfu.zip dfu/${FIRMWARE_DFU_ZIP}
			COMMAND ${CMAKE_COMMAND} -E remove ${KEY_FILE}
			COMMAND bash -c "sha1sum dfu/${FIRMWARE_DFU_ZIP} | cut -f1 -d' ' > dfu/${FIRMWARE_DFU_ZIP}.sha1" VERBATIM
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMENT "Generate DFU package with Softdevice, bootloader, and application"
			DEPENDS extract_sd_id
			)
		
		add_custom_target(generate_dfu_package_application
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "** Use nrfutil to generate DFU package"
			COMMAND ${CMAKE_COMMAND} -E echo "     Application: ${PROJECT_NAME}.hex"
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND bash -c "echo \"Softdevice id: $(cat softdevice_id)\"" VERBATIM
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "** Use pass to get encrypted key file, using entry: ${PASS_FILE}"
			COMMAND ${CMAKE_COMMAND} -E touch ${KEY_FILE}
			COMMAND pass ${PASS_FILE} > ${KEY_FILE}
			COMMAND ${CMAKE_COMMAND} -E echo "     Temp key file: ${KEY_FILE}"
			COMMAND bash -c "echo 'nrfutil pkg generate --hw-version 52 --sd-req $(cat softdevice_id) --key-file ${KEY_FILE} --application ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex --application-version ${FIRMWARE_DFU_VERSION} application_dfu.zip'" VERBATIM USES_TERMINAL
			COMMAND bash -c "nrfutil pkg generate --hw-version 52 --sd-req $(cat softdevice_id) --key-file ${KEY_FILE} --application ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex --application-version ${FIRMWARE_DFU_VERSION} application_dfu.zip" VERBATIM USES_TERMINAL
			COMMAND ${CMAKE_COMMAND} -E make_directory dfu
			COMMAND ${CMAKE_COMMAND} -E copy application_dfu.zip dfu/${APPLICATION_DFU_ZIP}
			COMMAND ${CMAKE_COMMAND} -E remove ${KEY_FILE}
			COMMAND bash -c "sha1sum dfu/${APPLICATION_DFU_ZIP} | cut -f1 -d' ' > dfu/${APPLICATION_DFU_ZIP}.sha1" VERBATIM
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMENT "Generate DFU package with application only"
			DEPENDS extract_sd_id
			)

		add_custom_target(generate_dfu_package_bootloader
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "** Use nrfutil to generate DFU package"
			COMMAND ${CMAKE_COMMAND} -E echo "     Bootloader:  bootloader.hex"
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND bash -c "echo \"Softdevice id: $(cat softdevice_id)\"" VERBATIM
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMAND ${CMAKE_COMMAND} -E echo "** Use pass to get encrypted key file, using entry: ${PASS_FILE}"
			COMMAND ${CMAKE_COMMAND} -E touch ${KEY_FILE}
			COMMAND pass ${PASS_FILE} > ${KEY_FILE}
			COMMAND ${CMAKE_COMMAND} -E echo "     Temp key file: ${KEY_FILE}"
			COMMAND bash -c "echo 'nrfutil pkg generate --hw-version 52 --sd-req $(cat softdevice_id) --key-file ${KEY_FILE} --bootloader ${CMAKE_BINARY_DIR}/bootloader/bootloader.hex --bootloader-version ${BOOTLOADER_DFU_VERSION} bootloader_dfu.zip'" VERBATIM USES_TERMINAL
			COMMAND bash -c "nrfutil pkg generate --hw-version 52 --sd-req $(cat softdevice_id) --key-file ${KEY_FILE} --bootloader ${CMAKE_BINARY_DIR}/bootloader/bootloader.hex --bootloader-version ${BOOTLOADER_DFU_VERSION} bootloader_dfu.zip" VERBATIM USES_TERMINAL
			COMMAND ${CMAKE_COMMAND} -E make_directory dfu
			COMMAND ${CMAKE_COMMAND} -E copy bootloader_dfu.zip dfu/${BOOTLOADER_DFU_ZIP}
			COMMAND ${CMAKE_COMMAND} -E remove ${KEY_FILE}
			COMMAND bash -c "sha1sum dfu/${BOOTLOADER_DFU_ZIP} | cut -f1 -d' ' > dfu/${BOOTLOADER_DFU_ZIP}.sha1" VERBATIM
			COMMAND ${CMAKE_COMMAND} -E echo
			COMMENT "Generate DFU package with bootloader only"
			DEPENDS extract_sd_id
			)
	


		if(NOT DEFINED FIRMWARE_RC_VERSION OR "${FIRMWARE_RC_VERSION}" STREQUAL "")
			set(RELEASE_REPOSITORY_DIRECTORY "${RELEASE_REPOSITORY}/firmwares/crownstone_${FIRMWARE_VERSION}")
		else()
			set(RELEASE_REPOSITORY_DIRECTORY "${RELEASE_CANDIDATE_REPOSITORY}/firmwares/crownstone_${FIRMWARE_VERSION}")
		endif()


		if(EXISTS ${RELEASE_REPOSITORY_DIRECTORY} )
			message(STATUS "Already released... No create_release_in_repository target available")
		else()

			add_custom_target(create_release_in_repository
					COMMAND ${CMAKE_COMMAND} -E echo
					COMMAND ${CMAKE_COMMAND} -E echo "** Copy to release repository \"${RELEASE_REPOSITORY_DIRECTORY}\""
					COMMAND ${CMAKE_COMMAND} -E make_directory ${RELEASE_REPOSITORY_DIRECTORY}/bin
					COMMAND ${CMAKE_COMMAND} -E echo "   Copy from ${CMAKE_BINARY_DIR}"
					COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/dfu/crownstone*.zip ${RELEASE_REPOSITORY_DIRECTORY}/bin
					COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/dfu/crownstone*.zip.sha1 ${RELEASE_REPOSITORY_DIRECTORY}/bin
					COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/bootloader/bootloader_settings.hex ${RELEASE_REPOSITORY_DIRECTORY}/bin/bootloader-settings.hex
					COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/crownstone.elf ${RELEASE_REPOSITORY_DIRECTORY}/bin
					COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/crownstone.hex ${RELEASE_REPOSITORY_DIRECTORY}/bin
					COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/crownstone.bin ${RELEASE_REPOSITORY_DIRECTORY}/bin
					COMMAND ${CMAKE_COMMAND} -E copy ${NRF5_DIR}/${SOFTDEVICE_HEX_FILE} ${RELEASE_REPOSITORY_DIRECTORY}/bin/softdevice.hex
					COMMAND ${CMAKE_COMMAND} -E echo "   Copy release config file from ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}"
					COMMAND ${CMAKE_COMMAND} -E make_directory ${RELEASE_REPOSITORY_DIRECTORY}/config
					COMMAND ${CMAKE_COMMAND} -E copy ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.config ${RELEASE_REPOSITORY_DIRECTORY}/config
					COMMENT "Copy all generated files to release repository"
					)
		endif()

		if(NOT DEFINED BOOTLOADER_RC_VERSION OR "${BOOTLOADER_RC_VERSION}" STREQUAL "")
			set(RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER "${RELEASE_REPOSITORY}/bootloaders/bootloader_${BOOTLOADER_VERSION}")
		else()
			set(RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER "${RELEASE_CANDIDATE_REPOSITORY}/bootloaders/bootloader_${BOOTLOADER_VERSION}")
		endif()


		if(EXISTS ${RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER} )
			message(STATUS "Already released... No create_bootloader_release_in_repository target available")
		else()

			add_custom_target(create_bootloader_release_in_repository
					COMMAND ${CMAKE_COMMAND} -E echo
					COMMAND ${CMAKE_COMMAND} -E echo "** Copy to release repository \"${RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER}\""
					COMMAND ${CMAKE_COMMAND} -E make_directory ${RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER}/bin
					COMMAND ${CMAKE_COMMAND} -E echo "   Copy from ${CMAKE_BINARY_DIR}"
					COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/dfu/bootloader*.zip ${RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER}/bin
					COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/dfu/bootloader*.zip.sha1 ${RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER}/bin
					COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/bootloader/bootloader.elf ${RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER}/bin
					COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/bootloader/bootloader.hex ${RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER}/bin
					COMMAND ${CMAKE_COMMAND} -E copy ${NRF5_DIR}/${SOFTDEVICE_HEX_FILE} ${RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER}/bin/softdevice.hex
					COMMAND ${CMAKE_COMMAND} -E echo "   Copy release config file from ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}"
					COMMAND ${CMAKE_COMMAND} -E make_directory ${RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER}/config
					COMMAND ${CMAKE_COMMAND} -E copy ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.config ${RELEASE_REPOSITORY_DIRECTORY_BOOTLOADER}/config
					COMMENT "Copy all generated files to release repository"
					)
		endif()

		set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${PROJECT_NAME}.bin)
		set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${PROJECT_NAME}.elf)
	
		set(FILE_MEMORY_LAYOUT "-Tgeneric_gcc_nrf52.ld")
		set(PATH_FILE_MEMORY "-L${CMAKE_SOURCE_DIR}/include/third/nrf/")
		set(PATH_FILE_MEMORY "${PATH_FILE_MEMORY} -L${NRF5_DIR}/modules/nrfx/mdk/")
		set(CMAKE_EXE_LINKER_FLAGS "${PATH_FILE_MEMORY} ${FILE_MEMORY_LAYOUT} ${CMAKE_EXE_LINKER_FLAGS}")
		target_link_libraries(${PROJECT_NAME} ${LIBS})

		# We actually do not need the file without extension
		#install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)

		# TARGETS are not accepted from other directoreis
		install(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin" DESTINATION .)
		install(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.elf" DESTINATION .)
		install(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex" DESTINATION .)
		install(FILES "${CMAKE_BINARY_DIR}/bootloader/bootloader.elf" DESTINATION .)
		install(FILES "${CMAKE_BINARY_DIR}/bootloader/bootloader.hex" DESTINATION .)
		if(EXISTS "${CMAKE_BINARY_DIR}/bootloader/bootloader_settings.hex")
			install(FILES "${CMAKE_BINARY_DIR}/bootloader/bootloader_settings.hex" DESTINATION .)
		endif()
		install(FILES "${NRF5_DIR}/${SOFTDEVICE_HEX_FILE}" DESTINATION .)
		install(DIRECTORY "${CMAKE_BINARY_DIR}/dfu"       DESTINATION . FILES_MATCHING PATTERN "*.zip" PATTERN "*.sha1")
	endif()
	
	set(FILE_LIST_VARIABLES "${CMAKE_BINARY_DIR}/list_variables.txt")
	file(WRITE ${FILE_LIST_VARIABLES} "")
	message(STATUS "Write variables to ${FILE_LIST_VARIABLES}")
	get_cmake_property(_variableNames VARIABLES)
	list(SORT _variableNames)
	foreach(_variableName ${_variableNames})
		file(APPEND "${FILE_LIST_VARIABLES}" "${_variableName}=${${_variableName}}\n")
	endforeach()


endif()
