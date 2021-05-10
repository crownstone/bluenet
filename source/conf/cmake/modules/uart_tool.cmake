include(${DEFAULT_MODULES_PATH}/load_configuration.cmake)

if(EXISTS "${DEFAULT_CONFIG_FILE}")
	load_configuration(${DEFAULT_CONFIG_FILE} CONFIG_LIST)
endif()

# Overwrite with runtime config
if(EXISTS "${CONFIG_FILE}")
	load_configuration(${CONFIG_FILE} CONFIG_LIST)
endif()

message(STATUS "UART device: ${UART_DEVICE}")
message(STATUS "Baudrate: ${UART_BAUDRATE}")

if(INSTRUCTION STREQUAL "START_MINICOM")
	message(STATUS "Start minicom")
	message(STATUS "minicom -b ${UART_BAUDRATE} -c on -D ${UART_DEVICE}")
	execute_process(
		COMMAND minicom -b ${UART_BAUDRATE} -c on -D ${UART_DEVICE}
		)
elseif(INSTRUCTION STREQUAL "START_BINARY_CLIENT")
	message(STATUS "Start binary client")
	message(STATUS "${DEFAULT_MODULES_PATH}/../../../../scripts/log-client.py -d ${UART_DEVICE}")
	execute_process(
		COMMAND ${DEFAULT_MODULES_PATH}/../../../../scripts/log-client.py -d ${UART_DEVICE}
		)
else()
	message(STATUS "Unknown instruction")
endif()
