include(${DEFAULT_MODULES_PATH}/load_configuration.cmake)

if(EXISTS "${DEFAULT_CONFIG_FILE}")
	message(STATUS "Load from default config file: ${DEFAULT_CONFIG_FILE}")
	load_configuration(${DEFAULT_CONFIG_FILE} CONFIG_LIST)
endif()

if(EXISTS "${TARGET_CONFIG_FILE}")
	message(STATUS "Load from target config file: ${TARGET_CONFIG_FILE}")
	load_configuration(${TARGET_CONFIG_FILE} CONFIG_LIST)
endif()

if(EXISTS "${TARGET_CONFIG_OVERWRITE_FILE}")
	message(STATUS "Load from target overwrite file: ${TARGET_CONFIG_OVERWRITE_FILE}")
	load_configuration(${TARGET_CONFIG_OVERWRITE_FILE} CONFIG_LIST)
endif()

# Overwrite with runtime config
if(EXISTS "${CONFIG_FILE}")
	message(STATUS "Load from runtime config file: ${CONFIG_FILE}")
	load_configuration(${CONFIG_FILE} CONFIG_LIST)
endif()

message(STATUS "Device: ${DEVICE}")
if(DEFINED SERIAL_NUM)
	message(STATUS "Serial number: ${SERIAL_NUM}")
endif()
message(STATUS "GDB port: ${GDB_PORT}")

if (SERIAL_NUM)
	set(SERIAL_NUM_GDB_SELECT "-select")
	set(SERIAL_NUM_GDB_PARAM "usb=${SERIAL_NUM}")
endif()

if(INSTRUCTION STREQUAL "START_GDB_SERVER")
	message(STATUS "Start GDB server")
	message(STATUS "JLinkGDBServer -Device ${DEVICE} ${SERIAL_NUM_GDB_SELECT} ${SERIAL_NUM_GDB_PARAM} -If SWD -speed 4000 -port ${GDB_PORT} -swoport ${SWO_PORT} -telnetport ${TELNET_PORT} -RTTTelnetPort ${RTT_PORT}")
	execute_process(
		COMMAND JLinkGDBServer -Device ${DEVICE} ${SERIAL_NUM_GDB_SELECT} ${SERIAL_NUM_GDB_PARAM} -If SWD -speed 4000 -port ${GDB_PORT} -swoport ${SWO_PORT} -telnetport ${TELNET_PORT} -RTTTelnetPort ${RTT_PORT}
		RESULT_VARIABLE status
		OUTPUT_VARIABLE output
		ERROR_VARIABLE error
		)
	message(STATUS "${output}")
elseif(INSTRUCTION STREQUAL "START_GDB_CLIENT")
	message(STATUS "Start GDB client")
	message(STATUS "${COMPILER_PATH}/bin/arm-none-eabi-gdb --command ${DEFAULT_CONF_GDB_PATH}/gdbinit --eval-command=\"target;remote;localhost:${GDB_PORT}\" --command ${DEFAULT_CONF_GDB_PATH}/gdbstart --exec=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.elf ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.elf")
	execute_process(
		COMMAND ${COMPILER_PATH}/bin/arm-none-eabi-gdb --command ${DEFAULT_CONF_GDB_PATH}/gdbinit --eval-command="target;remote;localhost:${GDB_PORT}" --command ${DEFAULT_CONFIG_GDB_PATH}/gdbstart --exec=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.elf ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.elf
		RESULT_VARIABLE status
		OUTPUT_VARIABLE output
		ERROR_VARIABLE error
		)
	message(STATUS "${output}")
	message(STATUS "${status}")
	message(STATUS "${error}")
else()
	message(STATUS "Unknown instruction")
endif()
