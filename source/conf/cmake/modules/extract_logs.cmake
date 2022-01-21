include(${DEFAULT_MODULES_PATH}/load_configuration.cmake)

if(EXISTS "${DEFAULT_CONFIG_FILE}")
	load_configuration(${DEFAULT_CONFIG_FILE} CONFIG_LIST)
endif()

# Overwrite with runtime config
if(EXISTS "${CONFIG_FILE}")
	load_configuration(${CONFIG_FILE} CONFIG_LIST)
endif()

if(INSTRUCTION STREQUAL "EXTRACT")
	message(STATUS "Extract logs from preprocessed source code in ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/crownstone.dir/src")
	execute_process(
		COMMAND ${DEFAULT_MODULES_PATH}/../../../../scripts/extract-log-strings.py --sourceFilesDir ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/crownstone.dir/src --topDir source/ --outputFile ${CMAKE_CURRENT_BINARY_DIR}/extracted_logs.json 
		)
else()
	message(STATUS "Unknown instruction")
endif()
