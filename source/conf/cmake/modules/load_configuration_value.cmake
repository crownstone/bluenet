function(load_configuration_value CONFIG_FILE CONFIG_NAME CONFIG_VALUE)
	if(EXISTS ${CONFIG_FILE})
		#message(STATUS "Load configuration file: ${CONFIG_FILE}")
		file(STRINGS ${CONFIG_FILE} ConfigContents)
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

				if(NOT Value STREQUAL "")
					# Strip double quotes
					string(REGEX REPLACE "\"" "" Value ${Value})
					string(STRIP "${Name}" Name)
					string(STRIP "${Value}" Value)
					#set(${Name} "${Value}")
					#message(STATUS "Found config value: ${Name}")
					if(Name STREQUAL "${CONFIG_NAME}")
						set(CONFIG_VALUE "${Value}" PARENT_SCOPE)
						break()
					endif()
				endif()
			endif()
		endforeach()
	else()
		message(STATUS "Cannot find configuration file: ${CONFIG_FILE}")
	endif()
endfunction()

