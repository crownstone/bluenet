####################################
### DEFAULT CONFIGURATION
####################################

SET(DEFAULT_CONFIGURATION_FILE "CMakeBuild.config.default")
SET(BLUENET_DIR "$ENV{BLUENET_DIR}")

IF(EXISTS ${BLUENET_DIR}/${DEFAULT_CONFIGURATION_FILE})
	file(STRINGS ${BLUENET_DIR}/${DEFAULT_CONFIGURATION_FILE} ConfigContents)
	foreach(NameAndValue ${ConfigContents})
		# Strip leading spaces
		string(REGEX REPLACE "^[ ]+" "" NameAndValue ${NameAndValue})
		# Ignore comments
		string(REGEX REPLACE "^\#.*" "" NameAndValue ${NameAndValue})

		if(NOT NameAndValue STREQUAL "")
			# Find variable name
			string(REGEX MATCH "^[^=]+" Name ${NameAndValue})

			# Find the value
			string(REPLACE "${Name}=" "" Value ${NameAndValue})
			# Set the variable
			set(${Name} "${Value}")
			MESSAGE(STATUS "Set default ${Name} to ${Value}")
		endif()
	endforeach()
else()
	MESSAGE(FATAL_ERROR "Could not find file ${BLUENET_DIR}/${DEFAULT_CONFIGURATION_FILE}!")
endif()

####################################
### ENVIRONMENT CONFIGURATION
####################################

SET(ENV_CONFIGURATION_FILE "CMakeBuild.config.local")

IF(EXISTS ${BLUENET_DIR}/${ENV_CONFIGURATION_FILE})
	file(STRINGS ${BLUENET_DIR}/${ENV_CONFIGURATION_FILE} ConfigContents)
	foreach(NameAndValue ${ConfigContents})
		# Strip leading spaces
		string(REGEX REPLACE "^[ ]+" "" NameAndValue ${NameAndValue})
		# Ignore comments
		string(REGEX REPLACE "^\#.*" "" NameAndValue ${NameAndValue})

		if(NOT NameAndValue STREQUAL "")
			# Find variable name
			string(REGEX MATCH "^[^=]+" Name ${NameAndValue})

			# Find the value
			string(REPLACE "${Name}=" "" Value ${NameAndValue})
			# Set the variable
			set(${Name} "${Value}")
			MESSAGE(STATUS "Overwrite ${Name} to ${Value}")
		endif()
	endforeach()
endif()

####################################
### SPECIFIC CONFIGURATION
####################################

SET(CONFIGURATION_FILE "CMakeBuild.config")
SET(CONFIG_DIR "$ENV{BLUENET_CONFIG_DIR}")
if(NOT EXISTS ${CONFIG_DIR}/${CONFIGURATION_FILE})
	SET(CONFIGURATION_FILE "CMakeBuild.config")
	SET(CONFIG_DIR ${CMAKE_SOURCE_DIR})
endif()

IF(EXISTS ${CONFIG_DIR}/${CONFIGURATION_FILE})
	file(STRINGS ${CONFIG_DIR}/${CONFIGURATION_FILE} ConfigContents)
	foreach(NameAndValue ${ConfigContents})
		# Strip leading spaces
		string(REGEX REPLACE "^[ ]+" "" NameAndValue ${NameAndValue})
		# Ignore comments
		string(REGEX REPLACE "^\#.*" "" NameAndValue ${NameAndValue})

		if(NOT NameAndValue STREQUAL "")
			# Find variable name
			string(REGEX MATCH "^[^=]+" Name ${NameAndValue})

			# Find the value
			string(REPLACE "${Name}=" "" Value ${NameAndValue})
			# Set the variable
			set(${Name} "${Value}")
			MESSAGE(STATUS "Overwrite ${Name} to ${Value}")
		endif()
	endforeach()
else()
	MESSAGE(FATAL_ERROR "Could not find file ${CONFIG_DIR}/${CONFIGURATION_FILE}, copy from ${CONFIGURATION_FILE}.default and adjust!")
endif()
