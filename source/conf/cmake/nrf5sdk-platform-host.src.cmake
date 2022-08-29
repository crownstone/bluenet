# defines a list of relative paths to source files for the nrf52 chip for the nordic nrf5 sdk.
# outvar: NRF5SDK_PLATFORM_HOST_SOURCE_REL

if(NRF5SDK_PLATFORM_HOST_SOURCE_REL)
	status (WARNING "NRF5SDK_PLATFORM_HOST_SOURCE_REL is non-empty.")
endif()

list(APPEND NRF5SDK_PLATFORM_HOST_SOURCE_REL "components/libraries/timer/app_timer_host.cpp")
list(APPEND NRF5SDK_PLATFORM_HOST_SOURCE_REL "components/softdevice/s132/mock/ble_gap.cpp")
list(APPEND NRF5SDK_PLATFORM_HOST_SOURCE_REL "components/softdevice/s132/mock/nrf_sdm.cpp")
list(APPEND NRF5SDK_PLATFORM_HOST_SOURCE_REL "components/softdevice/s132/mock/nrf_soc.cpp")