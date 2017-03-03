include_directories ( "include" )

INCLUDE_DIRECTORIES(${INCLUDE_DIR})

# Add to the -I flags
INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/ble/common")
#INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/ble/ble_services")
#INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/ble/device_manager")
INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/device/")
INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/drivers_nrf/common/")
INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/drivers_nrf/gpiote/")
INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/drivers_nrf/delay/")
INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/drivers_nrf/hal/")
INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/drivers_nrf/ppi/")
INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/drivers_nrf/pstorage/")
INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/drivers_nrf/timer/")
INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/drivers_nrf/saadc/")
#INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/drivers_nrf/comp/")
#INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/libraries/pwm")
INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/libraries/util/")
INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/libraries/timer/")
INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/libraries/scheduler/")
INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/softdevice/common/softdevice_handler/")
INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/softdevice/s132/headers/")
INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/toolchain/")
INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/toolchain/gcc")
INCLUDE_DIRECTORIES("${NRF51822_DIR}/components/toolchain/CMSIS/Include")

MESSAGE(STATUS "[test/adc_test/CMakeLists.txt]: Compiler: ${CMAKE_C_COMPILER}") 

set(TEST test_adc)

set(TEST_SOURCE_DIR "test/nrf5")

LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/drivers_nrf/timer/nrf_drv_timer.c")
LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/drivers_nrf/saadc/nrf_drv_saadc.c")

# lpcomp should not be here
LIST(APPEND NORDIC_SOURCE "${NRF51822_DIR}/components/drivers_nrf/comp/nrf_drv_comp.c")

# RTC1_IRQHandler
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/third/nrf/app_timer.c")

list(APPEND FOLDER_SOURCE "${NORDIC_SOURCE}")   
list(APPEND FOLDER_SOURCE "${SOURCE_DIR}/cs_sysNrf51.c") 
list(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_Syscalls.c") 
list(APPEND FOLDER_SOURCE "${SOURCE_DIR}/cfg/cs_Boards.c")
list(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_Error.cpp")   
list(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_BleError.cpp")   

list(APPEND FOLDER_SOURCE "${SOURCE_DIR}/drivers/cs_ADC.cpp")

set(TEST_SOURCE_FILES "${FOLDER_SOURCE}")

set(SOURCE_FILES ${TEST_SOURCE_DIR}/${TEST}.c ${TEST_SOURCE_FILES}) 
add_executable(${TEST} ${SOURCE_FILES})
add_test(NAME ${TEST} COMMAND ${TEST})
