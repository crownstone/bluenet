if(COMMAND cmake_policy)
	cmake_policy(SET CMP0054 NEW)
endif()

# This message should not list log output (will not be good for scripts parsing the output)

if(${DEFAULT_MODULES_PATH})
	include(${DEFAULT_MODULES_PATH}/hex.cmake)
else()
	get_filename_component(DEFAULT_MODULES_PATH ${CMAKE_SCRIPT_MODE_FILE} DIRECTORY)
endif()

include(${DEFAULT_MODULES_PATH}/load_configuration_value.cmake)

if(NOT DEFINED CONFIG_NAME)
	message(FATAL_ERROR "Need the name, \${CONFIG_NAME}, of the config value to get")
endif()

# Overwrite with runtime config
if(DEFINED DEFAULT_CONFIG_FILE)
	if(EXISTS "${DEFAULT_CONFIG_FILE}")
		load_configuration_value(${DEFAULT_CONFIG_FILE} ${CONFIG_NAME} CONFIG_VALUE)
	endif()
endif()

# Overwrite with runtime config
if(DEFINED CONFIG_FILE)
	if(EXISTS "${CONFIG_FILE}")
		load_configuration_value(${CONFIG_FILE} ${CONFIG_NAME} CONFIG_VALUE)
	endif()
endif()

message("${CONFIG_VALUE}")
