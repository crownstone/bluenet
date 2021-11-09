execute_process(
	#COMMAND ${Python_EXECUTABLE} -m pip install ${PIP_PACKAGE}
	COMMAND python3 -m pip install ${PIP_PACKAGE}
	RESULT_VARIABLE status
	OUTPUT_VARIABLE output
	ERROR_VARIABLE error
	)

message(STATUS "Output: ${output}")
if (error)
	message(STATUS "Error: ${error}")
endif()
if (status)
	message(STATUS "Status: ${status}")
endif()
