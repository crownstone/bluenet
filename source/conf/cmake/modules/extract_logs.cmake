include(${DEFAULT_MODULES_PATH}/load_configuration.cmake)

if(EXISTS "${DEFAULT_CONFIG_FILE}")
	load_configuration(${DEFAULT_CONFIG_FILE} CONFIG_LIST)
endif()

# Overwrite with runtime config
if(EXISTS "${CONFIG_FILE}")
	load_configuration(${CONFIG_FILE} CONFIG_LIST)
endif()

if(INSTRUCTION STREQUAL "EXTRACT")
	message(STATUS "WORKSPACE_DIR=${WORKSPACE_DIR}")
	message(STATUS "CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}")
	message(STATUS "CMAKE_CURRENT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}")
	message(STATUS "BOARD_TARGET=${BOARD_TARGET}")
	execute_process(
		COMMAND ${DEFAULT_MODULES_PATH}/../../../../scripts/extract-log-strings.py --sourceFilesDir ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/crownstone.dir/src --topDir source/ --outputFile ${CMAKE_CURRENT_BINARY_DIR}/extracted_logs.json 
		)
else()
	message(STATUS "Unknown instruction")
endif()
