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
		# Find variable name
		string(REGEX MATCH "^[^=]+" Name ${NameAndValue})
		# Find the value
		string(REPLACE "${Name}=" "" Value ${NameAndValue})
		# Set the variable
		set(${Name} "${Value}")
	endforeach()
else()
	MESSAGE(FATAL_ERROR "Could not find file ${CONFIG_DIR}/${CONFIGURATION_FILE}, copy from ${CONFIGURATION_FILE}.default and adjust!")
endif()
