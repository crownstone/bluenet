# We add the source files explicitly. This is recommended in the cmake system and will also force us all the time to
# consider the size of the final binary. Do not include things, if not necessary!

# Files are added to the list variable NORDIC_SOURCE_NRF5_REL relative to NRF5_DIR,
# this is in order for host fork of the nrf library to be able to fork this file as well.

if(NOT BUILD_MESHING)
	list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/timer/app_timer.c")
endif()

if(NOT ${COMPILE_FOR_HOST})
	if(DEVICE STREQUAL "nRF52832_xxAA")
		list(APPEND NORDIC_SOURCE_NRF5_REL "modules/nrfx/mdk/gcc_startup_nrf52.S")
		set_property(SOURCE "${NRF5_DIR}/modules/nrfx/mdk/gcc_startup_nrf52.S" PROPERTY LANGUAGE C)
	elseif(DEVICE STREQUAL "nRF52833")
		list(APPEND NORDIC_SOURCE_NRF5_REL "modules/nrfx/mdk/gcc_startup_nrf52833.S")
		set_property(SOURCE "${NRF5_DIR}/modules/nrfx/mdk/gcc_startup_nrf52833.S" PROPERTY LANGUAGE C)
	elseif(DEVICE STREQUAL "nRF52840_xxAA")
		list(APPEND NORDIC_SOURCE_NRF5_REL "modules/nrfx/mdk/gcc_startup_nrf52840.S")
		set_property(SOURCE "${NRF5_DIR}/modules/nrfx/mdk/gcc_startup_nrf52840.S" PROPERTY LANGUAGE C)
	else()
		message(FATAL_ERROR "Unkown device: ${DEVICE}")
	endif()
else()
	message(STATUS "no startup file added, compiling for host.")
endif()

# The following files are only added for the logging module by Nordic. It might be good to remove these files to
# save space in production. It should then be enclosed within a macro.
# Those files are: nrf_strerror.c

list(APPEND NORDIC_SOURCE_NRF5_REL "components/ble/ble_db_discovery/ble_db_discovery.c")
list(APPEND NORDIC_SOURCE_NRF5_REL "components/ble/common/ble_advdata.c")
list(APPEND NORDIC_SOURCE_NRF5_REL "components/ble/common/ble_srv_common.c")
list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/atomic/nrf_atomic.c")
list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/atomic_fifo/nrf_atfifo.c")


list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/experimental_section_vars/nrf_section_iter.c")
list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/fds/fds.c")
list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/fstorage/nrf_fstorage.c")
list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/fstorage/nrf_fstorage_sd.c")
list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/hardfault/hardfault_implementation.c")
list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/hardfault/nrf52/handler/hardfault_handler_gcc.c")

list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/strerror/nrf_strerror.c")
list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/util/app_error.c")
list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/util/app_error_handler_gcc.c")
#	list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/util/app_error_weak.c")


list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/util/nrf_assert.c")
list(APPEND NORDIC_SOURCE_NRF5_REL "components/softdevice/common/nrf_sdh.c")
list(APPEND NORDIC_SOURCE_NRF5_REL "components/softdevice/common/nrf_sdh_ble.c")
list(APPEND NORDIC_SOURCE_NRF5_REL "components/softdevice/common/nrf_sdh_soc.c")
list(APPEND NORDIC_SOURCE_NRF5_REL "modules/nrfx/drivers/src/nrfx_comp.c")
list(APPEND NORDIC_SOURCE_NRF5_REL "modules/nrfx/drivers/src/nrfx_wdt.c")
list(APPEND NORDIC_SOURCE_NRF5_REL "modules/nrfx/drivers/src/prs/nrfx_prs.c")
list(APPEND NORDIC_SOURCE_NRF5_REL "modules/nrfx/hal/nrf_nvmc.c")
# should be our own code, but SystemInit here contains a lot of PANs we don't have to solve subsequently...
list(APPEND NORDIC_SOURCE_NRF5_REL "modules/nrfx/mdk/system_nrf52.c")

if(NORDIC_SDK_VERSION GREATER 15)
	list(APPEND NORDIC_SOURCE_NRF5_REL "components/ble/nrf_ble_gq/nrf_ble_gq.c")
	list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/queue/nrf_queue.c")
endif()

if(CS_SERIAL_NRF_LOG_ENABLED)
	message(STATUS "SERIAL from NORDIC enabled")
	list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/balloc/nrf_balloc.c")
	list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/log/src/nrf_log_backend_serial.c")
	list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/log/src/nrf_log_default_backends.c")
	list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/log/src/nrf_log_frontend.c")
	list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/log/src/nrf_log_str_formatter.c")
	list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/memobj/nrf_memobj.c")
	list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/ringbuf/nrf_ringbuf.c")
	list(APPEND NORDIC_SOURCE_NRF5_REL "external/fprintf/nrf_fprintf.c")
	list(APPEND NORDIC_SOURCE_NRF5_REL "external/fprintf/nrf_fprintf_format.c")
	if(CS_SERIAL_NRF_LOG_ENABLED STREQUAL 1)
		list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/log/src/nrf_log_backend_rtt.c")
		list(APPEND NORDIC_SOURCE_NRF5_REL "external/segger_rtt/SEGGER_RTT.c")
		list(APPEND NORDIC_SOURCE_NRF5_REL "external/segger_rtt/SEGGER_RTT_printf.c")
	endif()
	if(CS_SERIAL_NRF_LOG_ENABLED STREQUAL 2)
		list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/log/src/nrf_log_backend_uart.c")
		list(APPEND NORDIC_SOURCE_NRF5_REL "integration/nrfx/legacy/nrf_drv_uart.c")
		list(APPEND NORDIC_SOURCE_NRF5_REL "modules/nrfx/drivers/src/nrfx_uart.c")
	endif()
else()
	if(NORDIC_SDK_VERSION GREATER 15)
		# Required now for gatt queue
		list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/memobj/nrf_memobj.c")
		list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/balloc/nrf_balloc.c")
	endif()
	message(STATUS "SERIAL from NORDIC disabled")
endif()

if(BUILD_MICROAPP_SUPPORT)
	list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/fstorage/nrf_fstorage.c")
	list(APPEND NORDIC_SOURCE_NRF5_REL "components/libraries/fstorage/nrf_fstorage_sd.c")
else()
	message(STATUS "Microapp support disabled, no fstorage module required")
endif()

if(BUILD_TWI)
	list(APPEND NORDIC_SOURCE_NRF5_REL "modules/nrfx/drivers/src/nrfx_twi.c")
else()
	message(STATUS "Module for twi disabled")
endif()

if(BUILD_GPIOTE)
	message(STATUS "Add Nordic C files for nrfx gpiote driver")
	list(APPEND NORDIC_SOURCE_NRF5_REL "modules/nrfx/drivers/src/nrfx_gpiote.c")
else()
	message(STATUS "Module for gpio tasks and events disabled")
endif()


