include_directories ( "include" )

MESSAGE(STATUS "[test/nrf5/test_generate]")

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

# Required for the UART backend
INCLUDE_DIRECTORIES("${NRF5_DIR}/integration/nrfx/legacy/")

MESSAGE(STATUS "[test/adc_test/CMakeLists.txt]: Compiler: ${CMAKE_C_COMPILER}")

set(TEST test_adc)

set(TEST_BLUENET_SRC_DIR "test/nrf5")

# Reset_Handler, etc.
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/mdk/gcc_startup_nrf52.S")
set_property(SOURCE "${NRF5_DIR}/modules/nrfx/mdk/gcc_startup_nrf52.S" PROPERTY LANGUAGE C)

# lpcomp should not be here
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/drivers/src/nrfx_comp.c")
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/scheduler/app_scheduler.c")
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/util/app_util_platform.c")

LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/crc16/crc16.c")

# In case of adding powersampling
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/fds/fds.c")
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/fstorage/nrf_fstorage.c")
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/fstorage/nrf_fstorage_sd.c")

# RTC1_IRQHandler
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/timer/app_timer.c")

LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/util/app_error_weak.c")

LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/softdevice/common/nrf_sdh.c")
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/softdevice/common/nrf_sdh_ble.c")

LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/atomic/nrf_atomic.c")
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/atomic_fifo/nrf_atfifo.c")
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/balloc/nrf_balloc.c")
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/experimental_section_vars/nrf_section_iter.c")

LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/memobj/nrf_memobj.c")
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/ringbuf/nrf_ringbuf.c")

LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/softdevice/common/nrf_sdh_soc.c")
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/drivers/src/nrfx_comp.c")
#	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/drivers/src/nrfx_saadc.c")
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/drivers/src/nrfx_uart.c")
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/drivers/src/prs/nrfx_prs.c")
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/drivers/src/nrfx_uarte.c")
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/hal/nrf_nvmc.c")
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/mdk/system_nrf51.c")
	
# Required for the UART backend
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/log/src/nrf_log_frontend.c")
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/log/src/nrf_log_default_backends.c")
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/log/src/nrf_log_backend_uart.c")
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/log/src/nrf_log_backend_serial.c")
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/components/libraries/log/src/nrf_log_str_formatter.c")
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/external/fprintf/nrf_fprintf.c")
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/external/fprintf/nrf_fprintf_format.c")
LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/integration/nrfx/legacy/nrf_drv_uart.c")
	

list(APPEND FOLDER_SOURCE "${NORDIC_SOURCE}")

LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/uart/cs_UartCommandHandler.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/uart/cs_UartConnection.cpp")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/uart/cs_UartHandler.cpp")

list(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/util/cs_Syscalls.c")
list(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/cfg/cs_Boards.c")
LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/common/cs_Handlers.cpp")
list(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/util/cs_Error.cpp")
list(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/util/cs_BleError.cpp")

list(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/drivers/cs_ADC.cpp")
list(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/drivers/cs_Serial.cpp")
list(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/drivers/cs_Timer.cpp")

list(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/ble/cs_Stack.cpp")

list(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/processing/cs_PowerSampling.cpp")
list(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/processing/cs_RecognizeSwitch.cpp")
list(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/storage/cs_State.cpp")
list(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/drivers/cs_Storage.cpp")
list(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/events/cs_EventDispatcher.cpp")

list(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/structs/cs_ScheduleEntriesAccessor.cpp")


list(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/third/SortMedian.cc")

# Somehow the following files are pulled in as well..., not nice..., should not be necessary
list(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/ble/cs_UUID.cpp")
list(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/protocol/cs_UartProtocol.cpp")
list(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/events/cs_EventDispatcher.cpp")

set(TEST_SOURCE_FILES "${FOLDER_SOURCE}")

set(SOURCE_FILES ${TEST_BLUENET_SRC_DIR}/${TEST}.cpp ${TEST_SOURCE_FILES}) 
add_executable(${TEST} ${SOURCE_FILES} ${TEST}.tmp)
add_test(NAME ${TEST} COMMAND ${TEST})

ADD_CUSTOM_COMMAND(OUTPUT ${TEST}.tmp
	COMMAND ${CMAKE_OBJCOPY_OVERLOAD} -j .text -j .data -O binary ${TEST} ${TEST}.tmp
	COMMAND ${CMAKE_OBJCOPY_OVERLOAD} -j .text -j .data -O ihex ${TEST} ${TEST}.hex
	COMMAND ${CMAKE_COMMAND} -E copy ${TEST}.tmp ${TEST}.bin
	COMMAND ${CMAKE_COMMAND} -E copy ${TEST} ${TEST}.elf
	COMMAND ${CMAKE_COMMAND} -E copy ${TEST}.hex crownstone.hex
	COMMAND ${CMAKE_COMMAND} -E copy ${TEST}.elf crownstone.elf
	COMMAND ${CMAKE_COMMAND} -E copy ${TEST}.bin crownstone.bin
	COMMAND ${CMAKE_SIZE} ${TEST}.elf
	COMMAND stat ${TEST}.bin | grep -i size | tr -d ':' |  tr -s ' ' | cut -f3 -d ' ' | xargs printf "** Size: %s bytes - totally available: 512kB - 110kB {softdevice} = 412kB. In dual bank: 206kB"
	COMMAND echo
	DEPENDS ${TEST}
	COMMENT "Object copy ${TEST} to ${TEST}.bin")

ADD_CUSTOM_TARGET(generate-dat-${TEST}
	COMMAND echo
	COMMAND echo "** Use srec_cat to create the INIT .dat file for the DFU"
	COMMAND echo
	COMMAND echo "srec_cat ${TEST}.hex -intel -exclude 0x3FC00 0x3FC20 -generate 0x3FC00 0x3FC04 -l-e-constant 0x01 4 -generate 0x3FC04 0x3FC08 -l-e-constant 0x00 4 -generate 0x3FC08 0x3FC0C -l-e-constant 0xFE 4 -generate 0x3FC0C 0x3FC20 -constant 0x00 -o ${TEST}.dat -intel"
	COMMAND srec_cat ${TEST}.hex -intel -exclude 0x3FC00 0x3FC20 -generate 0x3FC00 0x3FC04 -l-e-constant 0x01 4 -generate 0x3FC04 0x3FC08 -l-e-constant 0x00 4 -generate 0x3FC08 0x3FC0C -l-e-constant 0xFE 4 -generate 0x3FC0C 0x3FC20 -constant 0x00 -o ${TEST}.dat -intel
	DEPENDS ${TEST}.tmp
	COMMENT "Generate the INIT .dat file for the DFU"
	)

ADD_CUSTOM_TARGET(combine-${TEST}
	COMMAND echo
	COMMAND echo "** Use srec_cat to create one combined softdevice+app file"
	COMMAND echo
	COMMAND echo "srec_cat ${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_HEX}/${SOFTDEVICE}_softdevice.hex -intel bootloader.hex -intel ${TEST}.bin -binary -offset ${APPLICATION_START_ADDRESS} -o combined.hex -intel"
	COMMAND srec_cat ${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_HEX}/${SOFTDEVICE}_softdevice.hex -intel bootloader.hex -intel ${TEST}.bin -binary -offset ${APPLICATION_START_ADDRESS} -o combined.hex -intel
	DEPENDS ${TEST}.tmp
	COMMENT "Combine as one hex file"
	)

ADD_CUSTOM_TARGET(analyze-${TEST}
	COMMAND echo
	COMMAND echo "** Show headers"
	COMMAND ${CMAKE_READELF} --headers ${TEST}.elf
	#COMMAND ${CMAKE_OBJDUMP} -h ${TEST}.elf
	COMMAND echo
	COMMAND echo "** Show largest symbols / functions"
	COMMAND ${CMAKE_NM} --print-size --size-sort --radix=d ${TEST}.elf
	COMMAND echo
	COMMAND echo "** Show size of sections"
	COMMAND ${CMAKE_SIZE} -A -d ${TEST}.elf
	DEPENDS ${TEST}.tmp
	COMMENT "Read headers ${TEST}.elf"
	)

#LIST(APPEND NORDIC_SOURCE_${TEST} "${NRF5_DIR}/components/drivers_nrf/pwm/nrf_drv_pwm.c")
SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${TEST}.bin)

TARGET_LINK_LIBRARIES(${TEST} ${LIBS})
INSTALL(TARGETS ${TEST} RUNTIME DESTINATION bin)

# Second test file

list(APPEND FOLDER_SOURCE "${NORDIC_SOURCE_${TEST}}")

MESSAGE(STATUS "${NORDIC_SOURCE_${TEST}}")

set(TEST test_generate)

set(TEST_SOURCE_FILES "${FOLDER_SOURCE}")

INCLUDE_DIRECTORIES("${NRF5_DIR}/components/drivers_nrf/pwm/")

set(SOURCE_FILES ${TEST_BLUENET_SRC_DIR}/${TEST}.cpp ${TEST_SOURCE_FILES}) 
add_executable(${TEST} ${SOURCE_FILES} ${TEST}.tmp)
add_test(NAME ${TEST} COMMAND ${TEST})

ADD_CUSTOM_COMMAND(OUTPUT ${TEST}.tmp
	COMMAND ${CMAKE_OBJCOPY_OVERLOAD} -j .text -j .data -O binary ${TEST} ${TEST}.tmp
	COMMAND ${CMAKE_OBJCOPY_OVERLOAD} -j .text -j .data -O ihex ${TEST} ${TEST}.hex
	COMMAND ${CMAKE_COMMAND} -E copy ${TEST}.tmp ${TEST}.bin
	COMMAND ${CMAKE_COMMAND} -E copy ${TEST} ${TEST}.elf
	COMMAND ${CMAKE_COMMAND} -E copy ${TEST}.hex crownstone.hex
	COMMAND ${CMAKE_COMMAND} -E copy ${TEST}.elf crownstone.elf
	COMMAND ${CMAKE_COMMAND} -E copy ${TEST}.bin crownstone.bin
	COMMAND ${CMAKE_SIZE} ${TEST}.elf
	COMMAND stat ${TEST}.bin | grep -i size | tr -d ':' |  tr -s ' ' | cut -f3 -d ' ' | xargs printf "** Size: %s bytes - totally available: 512kB - 110kB {softdevice} = 412kB. In dual bank: 206kB"
	COMMAND echo
	DEPENDS ${TEST}
	COMMENT "Object copy ${TEST} to ${TEST}.bin")

ADD_CUSTOM_TARGET(generate-dat-${TEST}
	COMMAND echo
	COMMAND echo "** Use srec_cat to create the INIT .dat file for the DFU"
	COMMAND echo
	COMMAND echo "srec_cat ${TEST}.hex -intel -exclude 0x3FC00 0x3FC20 -generate 0x3FC00 0x3FC04 -l-e-constant 0x01 4 -generate 0x3FC04 0x3FC08 -l-e-constant 0x00 4 -generate 0x3FC08 0x3FC0C -l-e-constant 0xFE 4 -generate 0x3FC0C 0x3FC20 -constant 0x00 -o ${TEST}.dat -intel"
	COMMAND srec_cat ${TEST}.hex -intel -exclude 0x3FC00 0x3FC20 -generate 0x3FC00 0x3FC04 -l-e-constant 0x01 4 -generate 0x3FC04 0x3FC08 -l-e-constant 0x00 4 -generate 0x3FC08 0x3FC0C -l-e-constant 0xFE 4 -generate 0x3FC0C 0x3FC20 -constant 0x00 -o ${TEST}.dat -intel
	DEPENDS ${TEST}.tmp
	COMMENT "Generate the INIT .dat file for the DFU"
	)

ADD_CUSTOM_TARGET(combine-${TEST}
	COMMAND echo
	COMMAND echo "** Use srec_cat to create one combined softdevice+app file"
	COMMAND echo
	COMMAND echo "srec_cat ${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_HEX}/${SOFTDEVICE}_softdevice.hex -intel bootloader.hex -intel ${TEST}.bin -binary -offset ${APPLICATION_START_ADDRESS} -o combined.hex -intel"
	COMMAND srec_cat ${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_HEX}/${SOFTDEVICE}_softdevice.hex -intel bootloader.hex -intel ${TEST}.bin -binary -offset ${APPLICATION_START_ADDRESS} -o combined.hex -intel
	DEPENDS ${TEST}.tmp
	COMMENT "Combine as one hex file"
	)

ADD_CUSTOM_TARGET(analyze-${TEST}
	COMMAND echo
	COMMAND echo "** Show headers"
	COMMAND ${CMAKE_READELF} --headers ${TEST}.elf
	#COMMAND ${CMAKE_OBJDUMP} -h ${TEST}.elf
	COMMAND echo
	COMMAND echo "** Show largest symbols / functions"
	COMMAND ${CMAKE_NM} --print-size --size-sort --radix=d ${TEST}.elf
	COMMAND echo
	COMMAND echo "** Show size of sections"
	COMMAND ${CMAKE_SIZE} -A -d ${TEST}.elf
	DEPENDS ${TEST}.tmp
	COMMENT "Read headers ${TEST}.elf"
	)

SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${TEST}.bin)

TARGET_LINK_LIBRARIES(${TEST} ${LIBS})
#INSTALL(TARGETS ${TEST} RUNTIME DESTINATION bin)

















list(APPEND FOLDER_SOURCE "${NORDIC_SOURCE_${TEST}}")

MESSAGE(STATUS "${NORDIC_SOURCE_${TEST}}")

set(TEST test_state)

set(TEST_SOURCE_FILES "${FOLDER_SOURCE}")

INCLUDE_DIRECTORIES("${NRF5_DIR}/components/drivers_nrf/pwm/")

set(SOURCE_FILES ${TEST_BLUENET_SRC_DIR}/${TEST}.cpp ${TEST_SOURCE_FILES}) 
add_executable(${TEST} ${SOURCE_FILES} ${TEST}.tmp)
add_test(NAME ${TEST} COMMAND ${TEST})

ADD_CUSTOM_COMMAND(OUTPUT ${TEST}.tmp
	COMMAND ${CMAKE_OBJCOPY_OVERLOAD} -j .text -j .data -O binary ${TEST} ${TEST}.tmp
	COMMAND ${CMAKE_OBJCOPY_OVERLOAD} -j .text -j .data -O ihex ${TEST} ${TEST}.hex
	COMMAND ${CMAKE_COMMAND} -E copy ${TEST}.tmp ${TEST}.bin
	COMMAND ${CMAKE_COMMAND} -E copy ${TEST} ${TEST}.elf
	COMMAND ${CMAKE_COMMAND} -E copy ${TEST}.hex crownstone.hex
	COMMAND ${CMAKE_COMMAND} -E copy ${TEST}.elf crownstone.elf
	COMMAND ${CMAKE_COMMAND} -E copy ${TEST}.bin crownstone.bin
	COMMAND ${CMAKE_SIZE} ${TEST}.elf
	COMMAND stat ${TEST}.bin | grep -i size | tr -d ':' |  tr -s ' ' | cut -f3 -d ' ' | xargs printf "** Size: %s bytes - totally available: 512kB - 110kB {softdevice} = 412kB. In dual bank: 206kB"
	COMMAND echo
	DEPENDS ${TEST}
	COMMENT "Object copy ${TEST} to ${TEST}.bin")

ADD_CUSTOM_TARGET(generate-dat-${TEST}
	COMMAND echo
	COMMAND echo "** Use srec_cat to create the INIT .dat file for the DFU"
	COMMAND echo
	COMMAND echo "srec_cat ${TEST}.hex -intel -exclude 0x3FC00 0x3FC20 -generate 0x3FC00 0x3FC04 -l-e-constant 0x01 4 -generate 0x3FC04 0x3FC08 -l-e-constant 0x00 4 -generate 0x3FC08 0x3FC0C -l-e-constant 0xFE 4 -generate 0x3FC0C 0x3FC20 -constant 0x00 -o ${TEST}.dat -intel"
	COMMAND srec_cat ${TEST}.hex -intel -exclude 0x3FC00 0x3FC20 -generate 0x3FC00 0x3FC04 -l-e-constant 0x01 4 -generate 0x3FC04 0x3FC08 -l-e-constant 0x00 4 -generate 0x3FC08 0x3FC0C -l-e-constant 0xFE 4 -generate 0x3FC0C 0x3FC20 -constant 0x00 -o ${TEST}.dat -intel
	DEPENDS ${TEST}.tmp
	COMMENT "Generate the INIT .dat file for the DFU"
	)

ADD_CUSTOM_TARGET(combine-${TEST}
	COMMAND echo
	COMMAND echo "** Use srec_cat to create one combined softdevice+app file"
	COMMAND echo
	COMMAND echo "srec_cat ${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_HEX}/${SOFTDEVICE}_softdevice.hex -intel bootloader.hex -intel ${TEST}.bin -binary -offset ${APPLICATION_START_ADDRESS} -o combined.hex -intel"
	COMMAND srec_cat ${SOFTDEVICE_DIR}/${SOFTDEVICE_DIR_HEX}/${SOFTDEVICE}_softdevice.hex -intel bootloader.hex -intel ${TEST}.bin -binary -offset ${APPLICATION_START_ADDRESS} -o combined.hex -intel
	DEPENDS ${TEST}.tmp
	COMMENT "Combine as one hex file"
	)

ADD_CUSTOM_TARGET(analyze-${TEST}
	COMMAND echo
	COMMAND echo "** Show headers"
	COMMAND ${CMAKE_READELF} --headers ${TEST}.elf
	#COMMAND ${CMAKE_OBJDUMP} -h ${TEST}.elf
	COMMAND echo
	COMMAND echo "** Show largest symbols / functions"
	COMMAND ${CMAKE_NM} --print-size --size-sort --radix=d ${TEST}.elf
	COMMAND echo
	COMMAND echo "** Show size of sections"
	COMMAND ${CMAKE_SIZE} -A -d ${TEST}.elf
	DEPENDS ${TEST}.tmp
	COMMENT "Read headers ${TEST}.elf"
	)

SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${TEST}.bin)

TARGET_LINK_LIBRARIES(${TEST} ${LIBS})
#INSTALL(TARGETS ${TEST} RUNTIME DESTINATION bin)
