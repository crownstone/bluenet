SET(CONFIGURATION_FILE "CMakeBuild.config")
IF(EXISTS ${CMAKE_SOURCE_DIR}/${CONFIGURATION_FILE})
	file(STRINGS ${CMAKE_SOURCE_DIR}/${CONFIGURATION_FILE} ConfigContents)
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
	MESSAGE(FATAL_ERROR "Could not find file ${CONFIGURATION_FILE}, copy from ${CONFIGURATION_FILE}.default and adjust!")
endif()

