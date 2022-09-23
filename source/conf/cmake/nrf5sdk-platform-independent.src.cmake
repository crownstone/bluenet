# defines a list of relative paths to platform independent source files for the nordic nrf5 sdk.
# outvar: NRF5SDK_PLATFORM_INDEPENDENT_SOURCE_REL

if(NRF5SDK_PLATFORM_INDEPENDENT_SOURCE_REL)
	status (WARNING "NRF5SDK_PLATFORM_INDEPENDENT_SOURCE_REL is non-empty.")
endif()

list(APPEND NRF5SDK_PLATFORM_INDEPENDENT_SOURCE_REL "components/libraries/crc16/crc16.c")
list(APPEND NRF5SDK_PLATFORM_INDEPENDENT_SOURCE_REL "components/libraries/crc32/crc32.c")

list(APPEND NRF5SDK_PLATFORM_INDEPENDENT_SOURCE_REL "components/libraries/scheduler/app_scheduler.c")
list(APPEND NRF5SDK_PLATFORM_INDEPENDENT_SOURCE_REL "components/libraries/util/app_util_platform.c")