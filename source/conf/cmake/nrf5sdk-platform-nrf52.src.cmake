# defines a list of relative paths to source files for the nrf52 chip for the nordic nrf5 sdk.
# outvar: NRF5SDK_PLATFORM_NRF52_SOURCE_REL

if(NRF5SDK_PLATFORM_NRF52_SOURCE_REL)
	status (WARNING "NRF5SDK_PLATFORM_NRF52_SOURCE_REL is non-empty.")
endif()

list(APPEND NRF5SDK_PLATFORM_NRF52_SOURCE_REL "components/libraries/util/app_util_platform.c")