CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# Set application shorthand
SET(APPLICATION_SHORTHAND "bootloader")

# Set name of project
SET(PROJECT_NAME ${APPLICATION_SHORTHAND})

# Start a project
PROJECT(${PROJECT_NAME}) 

# Enabling C and CXX might lead to problems because we are cross-compiling, check in later CMake versions of this is
# still the case
#enable_language(CXX)
#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED on)
#set(CMAKE_CXX_EXTENSIONS OFF)

# The directory with some of the FindXXX modules
#SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/conf;${CMAKE_SOURCE_DIR}/conf/cmake")
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${DEFAULT_MODULES_PATH}")
MESSAGE(STATUS "bootloader: Search for FindX files in ${CMAKE_MODULE_PATH}")

#######################################################################################################################

SET(VERBOSITY                                "4"           CACHE STRING "Verbosity")
SET(HOST_TARGET                              "ON"          CACHE BOOL "Compile for host platform")
SET(NRF5_DIR                                 "unset"       CACHE STRING "The nRF5 sdk directory")
SET(WORKSPACE_DIR                            "unset"       CACHE STRING "The workspace directory")
SET(CONFIG_DIR                               "config"      CACHE STRING "The config main directory")
SET(BOARD_TARGET                             "unset"       CACHE STRING "The board target")
SET(BOOTLOADER_START_ADDRESS                 "unset"       CACHE STRING "Start address of bootloader")
SET(BOOTLOADER_LENGTH                        "unset"       CACHE STRING "Size of bootloader")

#######################################################################################################################
# Show variables
#######################################################################################################################

MESSAGE(STATUS "bootloader: Set verbosity to level: ${VERBOSITY}")
MESSAGE(STATUS "bootloader: Compiler: ${CMAKE_C_COMPILER}")

#######################################################################################################################
# Include your own FindX cmake files and options in the following file!
#######################################################################################################################

string(TIMESTAMP COMPILATION_DAY "%Y-%m-%d")

include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)

#######################################################################################################################

# Libraries (math, stdc++ libraries, etc.) 
# Note! We CANNOT use the size-constrained *_s versions! anymore
# We don't seem to use any libraries anymore... except for including everything that comes with -std=c+11
#SET(LIBS "-lc ${LIBRARY_RUNTIME} ${LIBRARY_MATH} -lstdc++")
SET(LIBS "")

IF (BUILD_MESH_BOOTLOADER)
	# Set the main file to use for this application
	SET(MAIN_FILE "${MESH_SDK_DIR}/mesh/bootloader/src/main.c")

	LIST(APPEND BOOTLOADER_SOURCE_FILES "${MESH_SDK_DIR}/mesh/bootloader/src/bootloader.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${MESH_SDK_DIR}/mesh/bootloader/src/bootloader_app_bridge.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${MESH_SDK_DIR}/mesh/bootloader/src/bootloader_info.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${MESH_SDK_DIR}/mesh/bootloader/src/bootloader_rtc.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${MESH_SDK_DIR}/mesh/bootloader/src/bootloader_util.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${MESH_SDK_DIR}/mesh/bootloader/src/dfu_bank.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${MESH_SDK_DIR}/mesh/bootloader/src/dfu_mesh.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${MESH_SDK_DIR}/mesh/bootloader/src/dfu_transfer_mesh.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${MESH_SDK_DIR}/mesh/bootloader/src/dfu_util.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${MESH_SDK_DIR}/mesh/bootloader/src/fifo.c")
#	LIST(APPEND BOOTLOADER_SOURCE_FILES "${MESH_SDK_DIR}/mesh/bootloader/src/main.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${MESH_SDK_DIR}/mesh/bootloader/src/mesh_packet.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${MESH_SDK_DIR}/mesh/bootloader/src/nrf_flash.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${MESH_SDK_DIR}/mesh/bootloader/src/radio_control.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${MESH_SDK_DIR}/mesh/bootloader/src/rand.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${MESH_SDK_DIR}/mesh/bootloader/src/transport.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${MESH_SDK_DIR}/mesh/bootloader/src/uECC.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/sha256/sha256.c")

	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${MESH_SDK_DIR}/mesh/bootloader/include")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/sha256")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/modules/nrfx/mdk")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/modules/nrfx/hal")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/modules/nrfx")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/integration/nrfx")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/toolchain/cmsis/include")


	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/util")
#	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/timer")
#	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/log")
#	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/log/src")
#	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/experimental_section_vars")
#	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/delay")

	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/softdevice/s132/headers")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/softdevice/s132/headers/nrf52")

	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/boards")

	# Reset_Handler, etc.
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/modules/nrfx/mdk/gcc_startup_nrf52.S")
	set_property(SOURCE "${NRF5_DIR}/modules/nrfx/mdk/gcc_startup_nrf52.S" PROPERTY LANGUAGE C)

	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/modules/nrfx/mdk/system_nrf52.c")

#	ADD_DEFINITIONS("-DNORDIC_SDK_VERSION=15")
	ADD_DEFINITIONS("-DBOOTLOADER")
	ADD_DEFINITIONS("-DRBC_MESH_PACKET_POOL_SIZE=32")
	ADD_DEFINITIONS("-DuECC_CURVE=uECC_secp256r1")
	ADD_DEFINITIONS("-DuECC_PLATFORM=uECC_arm")

	# For debug:
	ADD_DEFINITIONS("-DRTT_LOG")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${MESH_SDK_DIR}/external/rtt/include")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${MESH_SDK_DIR}/external/rtt/src/SEGGER_RTT.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${MESH_SDK_DIR}/external/rtt/src/SEGGER_RTT_printf.c")

	# For serial DFU interface:
	ADD_DEFINITIONS("-DRBC_MESH_SERIAL=1")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${MESH_SDK_DIR}/mesh/bootloader/src/serial_handler_uart.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${MESH_SDK_DIR}/mesh/bootloader/src/mesh_aci.c")

ELSE()
	# Set the main file to use for this application
	#SET(MAIN_FILE "${NRF5_DIR}/examples/dfu/secure_bootloader/main.c")
	SET(MAIN_FILE "${CMAKE_SOURCE_DIR}/bootloader/main.c")

	LIST(APPEND BOOTLOADER_SOURCE_FILES "${CMAKE_SOURCE_DIR}/bootloader/dfu_public_key.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${CMAKE_SOURCE_DIR}/src/cfg/cs_Boards.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${CMAKE_SOURCE_DIR}/bootloader/dev_info_service.c")

	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/modules/nrfx/mdk/gcc_startup_nrf52.S")
	set_property(SOURCE "${NRF5_DIR}/modules/nrfx/mdk/gcc_startup_nrf52.S" PROPERTY LANGUAGE C)
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/modules/nrfx/mdk/system_nrf52.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/ble/ble_services/ble_dis/ble_dis.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/util/app_error_weak.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/scheduler/app_scheduler.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/util/app_util_platform.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/crc32/crc32.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/mem_manager/mem_manager.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/util/nrf_assert.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/atomic_fifo/nrf_atfifo.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/atomic/nrf_atomic.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/balloc/nrf_balloc.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/fstorage/nrf_fstorage.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/fstorage/nrf_fstorage_nvmc.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/fstorage/nrf_fstorage_sd.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/queue/nrf_queue.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/experimental_section_vars/nrf_section_iter.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/strerror/nrf_strerror.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/sha256/sha256.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/crypto/backend/micro_ecc/micro_ecc_backend_ecc.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/crypto/backend/micro_ecc/micro_ecc_backend_ecdh.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/crypto/backend/micro_ecc/micro_ecc_backend_ecdsa.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/modules/nrfx/hal/nrf_nvmc.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/crypto/nrf_crypto_ecc.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/crypto/nrf_crypto_ecdsa.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/crypto/nrf_crypto_hash.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/crypto/nrf_crypto_init.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/crypto/nrf_crypto_shared.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/ble/common/ble_srv_common.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${CMAKE_SOURCE_DIR}/bootloader/nrf_bootloader.c")
#	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/bootloader/nrf_bootloader.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/bootloader/nrf_bootloader_app_start.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/bootloader/nrf_bootloader_app_start_final.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/bootloader/nrf_bootloader_dfu_timers.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/bootloader/nrf_bootloader_fw_activation.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/bootloader/nrf_bootloader_info.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/bootloader/nrf_bootloader_wdt.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/external/nano-pb/pb_common.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/external/nano-pb/pb_decode.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/crypto/backend/nrf_sw/nrf_sw_backend_hash.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/bootloader/dfu/dfu-cc.pb.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/bootloader/dfu/nrf_dfu.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${CMAKE_SOURCE_DIR}/bootloader/nrf_dfu_ble.c")
#	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/bootloader/ble_dfu/nrf_dfu_ble.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/bootloader/dfu/nrf_dfu_flash.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/bootloader/dfu/nrf_dfu_handling_error.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/bootloader/dfu/nrf_dfu_mbr.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/bootloader/dfu/nrf_dfu_req_handler.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/bootloader/dfu/nrf_dfu_settings.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/bootloader/dfu/nrf_dfu_settings_svci.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/bootloader/dfu/nrf_dfu_transport.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/bootloader/dfu/nrf_dfu_utils.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/bootloader/dfu/nrf_dfu_validation.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/bootloader/dfu/nrf_dfu_ver_validation.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/bootloader/dfu/nrf_dfu_svci_handler.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/svc/nrf_svc_handler.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/softdevice/common/nrf_sdh.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/softdevice/common/nrf_sdh_ble.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/softdevice/common/nrf_sdh_soc.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/crypto/backend/oberon/oberon_backend_chacha_poly_aead.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/crypto/backend/oberon/oberon_backend_ecc.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/crypto/backend/oberon/oberon_backend_ecdh.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/crypto/backend/oberon/oberon_backend_ecdsa.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/crypto/backend/oberon/oberon_backend_eddsa.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/crypto/backend/oberon/oberon_backend_hash.c")
	LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/crypto/backend/oberon/oberon_backend_hmac.c")

	MESSAGE(STATUS "bootloader: nrf log: ${CS_SERIAL_NRF_LOG_ENABLED}")
	IF (CS_SERIAL_NRF_LOG_ENABLED)
		MESSAGE(STATUS "bootloader: SERIAL log enabled")
		LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/log/src/nrf_log_backend_serial.c")
		LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/log/src/nrf_log_default_backends.c")
		LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/log/src/nrf_log_frontend.c")
		LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/log/src/nrf_log_str_formatter.c")
		LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/memobj/nrf_memobj.c")
		LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/ringbuf/nrf_ringbuf.c")
		LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/external/fprintf/nrf_fprintf.c")
		LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/external/fprintf/nrf_fprintf_format.c")
		IF (CS_SERIAL_NRF_LOG_ENABLED STREQUAL 1)
			LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/log/src/nrf_log_backend_rtt.c")
			LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/external/segger_rtt/SEGGER_RTT.c")
		ENDIF()
		IF (CS_SERIAL_NRF_LOG_ENABLED STREQUAL 2)
			LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/components/libraries/log/src/nrf_log_backend_uart.c")
			LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/integration/nrfx/legacy/nrf_drv_uart.c")
			LIST(APPEND BOOTLOADER_SOURCE_FILES "${NRF5_DIR}/modules/nrfx/drivers/src/nrfx_uart.c")
		ENDIF()
	ELSE()
		MESSAGE(STATUS "bootloader: SERIAL log disabled")
	ENDIF()
	
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/include")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/bootloader")
	
	# SDK config
	#LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/examples/dfu/secure_bootloader/pca10040_ble_debug/config")
	#LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/examples/dfu/secure_bootloader/pca10040_ble/config")
	
	
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/ble/common")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/boards")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/ble/ble_services/ble_dis")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/atomic")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/atomic_fifo")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/balloc")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/bootloader")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/bootloader/ble_dfu")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/bootloader/dfu")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/crc32")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/crypto")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/crypto/backend/cc310")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/crypto/backend/cc310_bl")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/crypto/backend/cifra")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/crypto/backend/mbedtls")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/crypto/backend/micro_ecc")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/crypto/backend/nrf_hw")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/crypto/backend/nrf_sw")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/crypto/backend/oberon")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/crypto/backend/optiga")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/delay")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/experimental_section_vars")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/fstorage")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/log")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/log/src")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/mem_manager")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/memobj")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/queue")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/ringbuf")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/sha256")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/scheduler")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/stack_info")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/strerror")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/svc")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/libraries/util")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/softdevice/common")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/softdevice/s132/headers")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/softdevice/s132/headers/nrf52")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/components/toolchain/cmsis/include")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/external/fprintf")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/external/micro-ecc/micro-ecc")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/external/nano-pb")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/external/nrf_oberon")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/external/nrf_oberon/include")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/integration/nrfx")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/modules/nrfx")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/modules/nrfx/hal")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/modules/nrfx/mdk")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/modules/nrfx/drivers/include")
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/external/segger_rtt")
	
	LIST(APPEND BOOTLOADER_INCLUDE_DIRS "${NRF5_DIR}/integration/nrfx/legacy/")

	# micro-eec
	# You first have to build the lib, see ${NRF5_DIR}/external/micro-ecc/build_all.sh
	# Make sure to set your compiler in ${NRF5_DIR}/components/toolchain/gcc/Makefile.posix
	# See https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.sdk5.v15.3.0/lib_crypto_backend_micro_ecc.html
	LIST(APPEND BOOTLOADER_LIBS "${NRF5_DIR}/external/micro-ecc/nrf52hf_armgcc/armgcc/micro_ecc_lib_nrf52.a")

	# oberon
	LIST(APPEND BOOTLOADER_LIBS "${NRF5_DIR}/external/nrf_oberon/lib/cortex-m4/hard-float/liboberon_2.0.7.a")

	ADD_DEFINITIONS("-DNRF_DFU_SETTINGS_VERSION=1")
	
ENDIF()

# Overwrite sdk config options
ADD_DEFINITIONS("-DUSE_APP_CONFIG")

ADD_DEFINITIONS("-DBOOTLOADER_VERSION=${BOOTLOADER_VERSION}")
		
message(STATUS "Configure cs_BootloaderConfig.cpp file")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cs_BootloaderConfig.c.in" "${CMAKE_CURRENT_BINARY_DIR}/cs_BootloaderConfig.c" @ONLY)
list(APPEND GENERATED_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/cs_BootloaderConfig.c")

ADD_EXECUTABLE(${PROJECT_NAME} ${MAIN_FILE} ${BOOTLOADER_SOURCE_FILES} ${GENERATED_SOURCES})

# Setting properties like this does not work, perhaps CMake finds it difficult to find the right compiler
# target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_11)

# Use .elf extension by default
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}" SUFFIX ".elf")

MESSAGE(STATUS "bootloader: Bootloader parameters: ${BOOTLOADER_START_ADDRESS}, ${BOOTLOADER_LENGTH}")

SET(CS_LINKER_FLAGS "-Wl,--defsym=BOOTLOADER_START_ADDRESS=${BOOTLOADER_START_ADDRESS} -Wl,--defsym=BOOTLOADER_LENGTH=${BOOTLOADER_LENGTH}")

SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${CS_LINKER_FLAGS}")

# Remove sections that are part of UICR (so objcopy does not generate very large files)
MESSAGE(STATUS "Remove sections .uicr_bootloader_start_address and .uicr_mbr_params_page (or objcopy will generate huge files)")
SET(REMOVE_SECTIONS --remove-section .uicr_bootloader_start_address --remove-section .uicr_mbr_params_page)

# Add post-build target to display size information
ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_SIZE} --format=berkeley "${PROJECT_NAME}.elf"
	COMMAND stat -t ${PROJECT_NAME}.elf | cut -f2 -d ' ' | xargs printf "** Firmware size : %s bytes"
	COMMAND ${CMAKE_COMMAND} -E echo ""
	#COMMAND stat ${PROJECT_NAME}.elf | grep -i size | tr -d ':' |  tr -s ' ' | cut -f3 -d ' ' | xargs printf "** Size: %s bytes")
	)

# Add post-build target to generate the .hex file (to be used in nrfjprog or jlink)
ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E echo "** bootloader: Remove UICR sections for start address and MBR parameters"
	COMMAND ${CMAKE_OBJCOPY_OVERLOAD} ${REMOVE_SECTIONS} -O ihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
	)
		
IF(SERIAL_NUM) 
	SET(SERIAL_NUM_SWITCH "--snr")
	SET(SERIAL_NUM_DPARAM "-DSERIAL_NUM:STRING=${SERIAL_NUM}")
ENDIF()

ADD_CUSTOM_TARGET(write_bootloader
	COMMAND ${CMAKE_COMMAND} -E echo
	COMMAND ${CMAKE_COMMAND} -E echo "** Use nrfjprog to write ${PROJECT_NAME}"
	COMMAND ${CMAKE_COMMAND} -E echo
	COMMAND nrfjprog -f nrf52 --program ${PROJECT_NAME}.hex --sectorerase ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}
	COMMENT "Flashes bootloader to remote target board"
	)
		
ADD_CUSTOM_TARGET(write_bootloader_address
	COMMAND ${CMAKE_COMMAND} -E echo
	COMMAND ${CMAKE_COMMAND} -E echo "** Use nrfjprog to write bootloader address ${BOOTLOADER_START_ADDRESS} to UICR"
	COMMAND ${CMAKE_COMMAND} -E echo
	COMMAND nrfjprog -f nrf52 --memwr 0x10001014 --val ${BOOTLOADER_START_ADDRESS} ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}
	COMMENT "Writes bootloader start addresss to UICR"
	)

# Some debug information
IF(VERBOSITY GREATER 4)
	MESSAGE(STATUS "bootloader: \"${PROJECT_NAME}\" uses the following CXX flags: \"${CMAKE_CXX_FLAGS}\" (can be empty)")
	MESSAGE(STATUS "bootloader: Include dirs: ${BOOTLOADER_INCLUDE_DIRS}")
	MESSAGE(STATUS "bootloader: Linked libraries: ${BOOTLOADER_LIBS}")
ENDIF()

MESSAGE(STATUS "bootloader: Target ${BOARD_TARGET}")

SET_SOURCE_FILES_PROPERTIES(${BOOTLOADER_SOURCE_FILES}
	PROPERTIES OBJECT_DEPENDS ${WORKSPACE_DIR}/${CONFIG_DIR}/${BOARD_TARGET}/CMakeBuild.config
)
	
SET(FILE_MEMORY_LAYOUT "-Tsecure_bootloader_gcc_nrf52.ld")
SET(PATH_FILE_MEMORY "-L${CMAKE_SOURCE_DIR}/bootloader/")
SET(PATH_FILE_MEMORY "${PATH_FILE_MEMORY} -L${NRF5_DIR}/modules/nrfx/mdk/")
SET(CMAKE_EXE_LINKER_FLAGS "${PATH_FILE_MEMORY} ${FILE_MEMORY_LAYOUT} ${CMAKE_EXE_LINKER_FLAGS}")

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC ${BOOTLOADER_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${BOOTLOADER_LIBS})
INSTALL(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${WORKSPACE_DIR}/bin/${BOARD_TARGET})

