include(${DEFAULT_MODULES_PATH}/hex.cmake)

set(offset_hex "${OFFSET}")
set(max_size_hex "${MAX_SIZE}")
set(binary "${BINARY}")

# Note, these commands are Linux only. Check for other OS how to retrieve this info
execute_process(COMMAND ${CMAKE_SIZE} "${binary}"
		TIMEOUT 60
		RESULT_VARIABLE status
		OUTPUT_VARIABLE contents
		ERROR_VARIABLE error
		OUTPUT_STRIP_TRAILING_WHITESPACE
		)
if (error)
	message(STATUS "Error: ${error}")
	message(STATUS "Status: ${status}")
endif()

string(REGEX REPLACE "\n" ";" contents "${contents}")

list(GET contents 1 sizes)

#message(STATUS "Sizes: ${sizes}")

string(REGEX REPLACE "\t" ";" sizes "${sizes}")

list(GET sizes 0 text_size)
list(GET sizes 1 data_size)
#message(STATUS "Text size: ${text_size}")
#message(STATUS "Data size: ${data_size}")

math(EXPR app_size "${text_size} + ${data_size}")
to_hex("${app_size}" app_size_hex "0x")

message(STATUS "The app is of size ${app_size} (${app_size_hex})")

from_hex("${offset_hex}" offset)
math(EXPR flash_size "${text_size} + ${data_size} + ${offset}")
to_hex("${flash_size}" flash_size_hex "0x")

message(STATUS "The app ends at address ${flash_size} (${flash_size_hex})")

from_hex("${max_size_hex}" max_size)

if(flash_size GREATER max_size)
	message(FATAL_ERROR "Size ${flash_size_hex} is beyond ${max_size_hex}")
else()
	message(STATUS "Size ${flash_size_hex} is below ${max_size_hex}. Good! This might fit.")
endif()

