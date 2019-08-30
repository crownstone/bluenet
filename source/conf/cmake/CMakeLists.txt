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
PROJECT(${PROJECT_NAME}) # ${TEST_COMPILERS})

# The directory with some of the FindXXX modules
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/conf;${CMAKE_SOURCE_DIR}/conf/cmake")
MESSAGE(STATUS "CMakeLists: Search for FindX files in ${CMAKE_MODULE_PATH}")

#######################################################################################################################

SET(VERBOSITY                                "4"           CACHE STRING "Verbosity")
SET(HOST_TARGET                              "ON"          CACHE BOOL "Compile for host platform")
SET(GIT_BRANCH                               "unset"       CACHE STRING "Current git branch")
SET(COMPILATION_DAY                          "unset"       CACHE STRING "Day of compilation")
SET(NRF5_DIR                                 "unset"       CACHE STRING "The nRF5 sdk directory")

#######################################################################################################################
# Show variables
#######################################################################################################################

MESSAGE(STATUS "CMakeLists: Set verbosity to level: ${VERBOSITY}")
MESSAGE(STATUS "CMakeLists: Git branch: ${GIT_BRANCH}")
MESSAGE(STATUS "CMakeLists: Day of compilation: ${COMPILATION_DAY}")
MESSAGE(STATUS "CMakeLists: Compiler: ${CMAKE_C_COMPILER}")

#######################################################################################################################
# Include your own FindX cmake files and options in the following file!
#######################################################################################################################


#######################################################################################################################

MESSAGE(STATUS "CMakeLists: Do not set application start address")
#MESSAGE(STATUS "CMakeLists: Set application start address in linker script to ${APPLICATION_START_ADDRESS}")
#CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/conf/nRF-softdevice.ld.in ${CMAKE_SOURCE_DIR}/conf/nRF-softdevice.ld)

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
SET(INCLUDE_DIR "include" "include/ble" "include/third" "include/third/nrf")

# TODO: Just only include th NRF5 directory and refer to the header files using that relative path in the source files
# Then we do not need to update the CMakeLists.txt file all the time
LIST(APPEND INCLUDE_DIR "${NRF5_DIR}")

LIST(APPEND INCLUDE_DIR "${NRF5_DIR}/components/toolchain/cmsis/include")
LIST(APPEND INCLUDE_DIR "${NRF5_DIR}/config")
LIST(APPEND INCLUDE_DIR "${NRF5_DIR}/components/libraries/fstorage")
LIST(APPEND INCLUDE_DIR "${NRF5_DIR}/components/libraries/experimental_section_vars")

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
			MESSAGE(STATUS "CMakeLists: Include Crownstone file: " ${crownstone_file})
		ENDFOREACH()
	ENDIF()

	# Add all header files without exception
	#FILE(GLOB FOLDER_HEADER ${INCLUDE_DIR}/*.h ${INCLUDE_DIR}/*.hh ${INCLUDE_DIR}/*.hpp)

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
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/scheduler/app_scheduler.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/strerror/nrf_strerror.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/util/app_error_weak.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/util/app_util_platform.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/softdevice/common/nrf_sdh.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/softdevice/common/nrf_sdh_ble.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/softdevice/common/nrf_sdh_soc.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/drivers/src/nrfx_comp.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/drivers/src/prs/nrfx_prs.c")
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/hal/nrf_nvmc.c")
	# should be our own code, but SystemInit here contains a lot of PANs we don't have to solve subsequently...
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/mdk/system_nrf52.c")
	
	IF (CS_SERIAL_NRF_LOG_ENABLED)
		MESSAGE(STATUS "CMakeLists: SERIAL from NORDIC enabled")
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
		MESSAGE(STATUS "CMakeLists: SERIAL from NORDIC disabled")
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
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/prov/src/nrf_mesh_prov_bearer_gatt.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/prov/src/nrf_mesh_prov.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/prov/src/prov_provisioner.c")

		# All C files in mesh/access/src
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/access/src/access_publish_retransmission.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/access/src/access.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/access/src/access_reliable.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/access/src/access_loopback.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/access/src/access_publish.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/access/src/device_state_manager.c")

		# All C files in mesh/dfu/src
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/dfu/src/nrf_mesh_dfu.c")

		# All C files in mesh/stack/src
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/stack/src/mesh_stack.c")

		# models/foundation/config/src/config_server.c
		# models/foundation/config/src/composition_data.c
		# models/foundation/config/src/packed_index_list.c
		# models/foundation/health/src/health_server.c
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/models/foundation/config/src/config_server.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/models/foundation/config/src/composition_data.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/models/foundation/config/src/packed_index_list.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/models/foundation/health/src/health_server.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/models/foundation/config/src/config_client.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/models/foundation/health/src/health_client.c")

		# Any other mesh models that are used in your application
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/models/model_spec/common/src/model_common.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/models/model_spec/generic_onoff/src/generic_onoff_client.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/models/model_spec/generic_onoff/src/generic_onoff_server.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/examples/common/src/app_onoff.c")

		# external/micro-ecc/uECC.c
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/external/micro-ecc/uECC.c")

#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/examples/common/src/assertion_handler_weak.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/examples/common/src/mesh_provisionee.c")
		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/examples/common/src/mesh_provisionee.c")

#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/serial/src/nrf_mesh_serial.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/serial/src/serial_handler_openmesh.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/serial/src/serial_handler_prov.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/serial/src/serial_handler_dfu.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/serial/src/serial_handler_app.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/serial/src/serial_handler_mesh.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/serial/src/serial_uart.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/serial/src/serial_handler_access.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/serial/src/serial_handler_config.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/serial/src/serial.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/serial/src/serial_bearer.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/serial/src/serial_handler_models.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/serial/src/serial_handler_common.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/serial/src/serial_handler_device.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_timeslot_timer.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_generic_level_server.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_proxy_vectors.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_advertiser.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_timer.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_generic_level_client.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_serial_handler_mesh.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_timer_scheduler.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_serial_handler_device.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_mesh_gatt.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_serial_handler_openmesh.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_generic_dtt_server.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_ad_listener.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_core_tx.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_proxy.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_composition_data.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_nrf_mesh.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_ccm_soft.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_bearer_event.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_generic_ponoff_client.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_mesh_stack.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_filters.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_timeslot.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_generic_dtt_client.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_packet_mgr.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_ccm_ad.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_serial_uart.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_mesh_config_backend.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/flash_manager_test_util.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_access_publish_retransmission.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/test_serial_bearer_common.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_prov_utils.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_heartbeat.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/test_assert.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_serial_handler_app.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/aes_soft.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_device_state_manager.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_core_tx_lpn.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_serial.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_prov_bearer_adv.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_provisioning.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_prov_provisioning.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_nrf_mesh_configure.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_packet_buffer.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_transport.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_event.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/packet_mgr_linux.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_serial_handler_models.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_radio_config.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_serial_pb_remote_client.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_serial_bearer_slip_enc.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_serial_bearer_simple.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_config_client.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_net_beacon.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_core_tx_adv.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_generic_onoff_client.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_health_client.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_radio.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_bearer_handler.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_lpn.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/mtt_packet_mgr.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_virtual_addressing.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_aes_cmac.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_serial_handler_config.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_network_vectors.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_msg_cache.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/proxy_test_common.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_mesh_lpn_subman.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_packet.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_network.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_bitfield.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_config_server.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_queue.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_access.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_msqueue.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_flash_manager.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_keygen.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_fifo.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_mesh_flash.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_mesh_config_flashman_glue.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_model_common.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_serial_packet.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_serial_handler_dfu.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_health_server.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_fsm.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_net_state.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_linked_list.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_transport_lpn.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_access_reliable.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_generic_ponoff_ss.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_flash_manager_defrag.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_net_packet.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_scanner.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_replay_cache.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_prov_beacon.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_generic_onoff_server.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_proxy_filter.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_access_publish.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_mesh_config.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_beacon.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_serial_handler_access.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/src/ut_enc.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/test/mttest/mttest.c")

		
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bootloader/src/bootloader.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bootloader/src/rand.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bootloader/src/dfu_transfer_mesh.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bootloader/src/bootloader_util.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bootloader/src/bootloader_rtc.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bootloader/src/bootloader_app_bridge.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bootloader/src/dfu_util.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bootloader/src/dfu_mesh.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bootloader/src/serial_handler_uart.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bootloader/src/mesh_packet.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bootloader/src/fifo.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bootloader/src/mesh_aci.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bootloader/src/nrf_flash.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bootloader/src/transport.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bootloader/src/main.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bootloader/src/radio_control.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bootloader/src/dfu_bank.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bootloader/src/bootloader_info.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/bootloader/src/uECC.c")

#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/gatt/src/proxy.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/gatt/src/proxy_filter.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/mesh/gatt/src/mesh_gatt.c")
		
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/external/rtt/src/SEGGER_RTT.c")
#		LIST(APPEND NORDIC_SOURCE "${MESH_SDK_DIR}/external/rtt/src/SEGGER_RTT_printf.c")
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
    		MESSAGE(FATAL_ERROR "Unknown mesh_mem backend \"${MESH_MEM_BACKEND}\"")
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
			MESSAGE(STATUS "CMakeLists: Include Nordic file: " ${nordic_file})
		ENDFOREACH()
	ENDIF()

	LIST(APPEND FOLDER_SOURCE ${NORDIC_SOURCE})

	# For Visual Studio group the files
	SOURCE_GROUP("Source files" FILES ${FOLDER_SOURCE})
	SOURCE_GROUP("Header files" FILES ${FOLDER_HEADER})

	INCLUDE_DIRECTORIES(${INCLUDE_DIR})

	# Add to the -I flags
	# This is required for the Nordic files internally. They refer only to header files without component paths.
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/ble/ble_services")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/ble/common")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/device/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/drivers_nrf/common/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/drivers_nrf/comp/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/libraries/atomic/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/libraries/atomic_fifo/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/libraries/balloc/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/libraries/crc16/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/libraries/delay/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/libraries/experimental_log/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/libraries/experimental_log/src/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/libraries/experimental_memobj/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/libraries/experimental_section_vars/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/libraries/fds/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/libraries/fstorage/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/libraries/log/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/libraries/log/src/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/libraries/memobj/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/libraries/ringbuf/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/libraries/scheduler/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/libraries/strerror/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/libraries/timer/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/libraries/trace/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/libraries/util/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/softdevice/common/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/softdevice/common/softdevice_handler/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/softdevice/s${SOFTDEVICE_SERIES}/headers")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/toolchain/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/toolchain/cmsis/include")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/toolchain/gcc/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/external/fprintf/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/integration/nrfx/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/modules/nrfx/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/modules/nrfx/drivers/include/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/modules/nrfx/hal/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/modules/nrfx/mdk/")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/external/segger_rtt")
	INCLUDE_DIRECTORIES("${NRF5_DIR}/integration/nrfx/legacy/")

	INCLUDE_DIRECTORIES("${NRF5_DIR}/components/softdevice/s132/headers/nrf52")

	IF (BUILD_MESHING)
		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/mesh/stack/api")
#		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/mesh/serial/api")
#		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/mesh/serial/include")
#		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/mesh/test/include")
#		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/mesh/test/include/device_wrappers")
#		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/mesh/test/mttest")
		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/mesh/prov/api")
		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/mesh/prov/include")
		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/mesh/access/api")
		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/mesh/access/include")
#		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/mesh/bootloader/include")
		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/mesh/dfu/api")
		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/mesh/dfu/include")
		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/mesh/core/api")
		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/mesh/core/include")
		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/mesh/bearer/api")
		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/mesh/bearer/include")
		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/mesh/gatt/api")
		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/mesh/gatt/include")
		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/models/foundation/config/include")
		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/models/foundation/health/include")
		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/models/model_spec/common/include")
		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/models/model_spec/generic_onoff/include")
		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/external/micro-ecc")
#		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/external/rtt/include")
		INCLUDE_DIRECTORIES("${MESH_SDK_DIR}/examples/common/include")
		IF (MESH_MEM_BACKEND STREQUAL "mem_manager")
			INCLUDE_DIRECTORIES("${NRF5_DIR}/components/libraries/mem_manager")
		ENDIF()
		
	ENDIF()

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
	SET(${SOFTDEVICE_DIR}, "${NRF5_DIR}/components/softdevice/s${SOFTDEVICE_SERIES}/")
	SET(${SOFTDEVICE_DIR_HEX}, "hex")
ENDIF()

IF((DEFINED TEST_TARGET) AND "${TEST_TARGET}" STREQUAL "\"nrf5\"")
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

		#SET(INCLUDE_ONLY_SECTIONS "-j .text -j .data -j .sdh_soc_observers -j .sdh_ble_observers -j .sdh_state_observers -j .sdh_stack_observers -j .sdh_req_observers -j .nrf_balloc")
		SET(INCLUDE_ONLY_SECTIONS "")

		# https://stackoverflow.com/questions/5278444/adding-a-custom-command-with-the-file-name-as-a-target
		ADD_CUSTOM_COMMAND(OUTPUT ${PROJECT_NAME}.tmp
			COMMAND ${CMAKE_OBJCOPY_OVERLOAD} ${INCLUDE_ONLY_SECTIONS} -O binary ${PROJECT_NAME} ${PROJECT_NAME}.tmp
			COMMAND ${CMAKE_OBJCOPY_OVERLOAD} ${INCLUDE_ONLY_SECTIONS} -O ihex ${PROJECT_NAME} ${PROJECT_NAME}.hex
			COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_NAME}.tmp ${PROJECT_NAME}.bin
			COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_NAME} ${PROJECT_NAME}.elf
			COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}.elf
			COMMAND stat ${PROJECT_NAME}.bin | grep -i size | tr -d ':' |  tr -s ' ' | cut -f3 -d ' ' | xargs printf "** Size: %s bytes - totally available: 512kB - 110kB {softdevice} - {bootloader} - {app storage} = 360kB . In dual bank: 180kB = DFU_IMAGE_MAX_SIZE_BANKED"
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

