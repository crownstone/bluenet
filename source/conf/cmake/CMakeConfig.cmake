####################################
### LOAD DIRECTORIES FROM ENV
####################################

SET(BLUENET_DIR "$ENV{BLUENET_DIR}")
SET(BLUENET_CONFIG_DIR "$ENV{BLUENET_CONFIG_DIR}")
SET(BLUENET_WORKSPACE_DIR "$ENV{BLUENET_WORKSPACE_DIR}")

####################################
### DEFAULT CONFIGURATION
####################################

SET(DEFAULT_CONFIGURATION_FILE "CMakeBuild.config.default")
SET(INSPECT_FILE ${BLUENET_DIR}/${DEFAULT_CONFIGURATION_FILE})

IF(EXISTS ${INSPECT_FILE})
	file(STRINGS ${INSPECT_FILE} ConfigContents)
	foreach(NameAndValue ${ConfigContents})
		# Strip leading spaces
		string(REGEX REPLACE "^[ ]+" "" NameAndValue ${NameAndValue})
		# Strip trailing spaces
		string(REGEX REPLACE "[ ]+$" "" NameAndValue "${NameAndValue}")
		# Ignore comments
		string(REGEX REPLACE "^\#.*" "" NameAndValue "${NameAndValue}")

		if(NOT NameAndValue STREQUAL "")
			# Find variable name
			string(REGEX MATCH "^[^=]+" Name ${NameAndValue})

			# Find the value
			string(REPLACE "${Name}=" "" Value ${NameAndValue})
			# Set the variable
			set(${Name} "${Value}")
			IF(VERBOSITY GREATER 4)
				MESSAGE(STATUS "CMakeConfig [${INSPECT_FILE}]: Set default ${Name} to ${Value}")
			ENDIF()
		endif()
	endforeach()
else()
	MESSAGE(FATAL_ERROR "Could not find file ${INSPECT_FILE}!")
endif()

####################################
### ENVIRONMENT CONFIGURATION
####################################

SET(ENV_CONFIGURATION_FILE "CMakeBuild.config.local")
SET(INSPECT_FILE ${BLUENET_DIR}/${ENV_CONFIGURATION_FILE})

IF(EXISTS ${INSPECT_FILE})
	file(STRINGS ${INSPECT_FILE} ConfigContents)
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
			IF(VERBOSITY GREATER 4)
				MESSAGE(STATUS "CMakeConfig [${INSPECT_FILE}]: Overwrite ${Name} to ${Value}")
			ENDIF()
		endif()
	endforeach()
endif()

####################################
### SPECIFIC CONFIGURATION
####################################

SET(CONFIGURATION_FILE "CMakeBuild.config")
if(NOT EXISTS ${BLUENET_CONFIG_DIR}/${CONFIGURATION_FILE})
	SET(CONFIGURATION_FILE "CMakeBuild.config")
	SET(BLUENET_CONFIG_DIR ${CMAKE_SOURCE_DIR})
endif()

SET(INSPECT_FILE ${BLUENET_CONFIG_DIR}/${CONFIGURATION_FILE})

IF(EXISTS ${INSPECT_FILE})
	file(STRINGS ${INSPECT_FILE} ConfigContents)
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
			IF(VERBOSITY GREATER 4)
				MESSAGE(STATUS "CMakeConfig [${INSPECT_FILE}]: Overwrite ${Name} to ${Value}")
			ENDIF()
		endif()
	endforeach()
else()
	MESSAGE(FATAL_ERROR "CMakeConfig: Could not find file ${INSPECT_FILE}, copy from ${CONFIGURATION_FILE}.default and adjust!")
endif()
