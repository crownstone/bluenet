include(${DEFAULT_MODULES_PATH}/hex.cmake)

function(get_mac_address SERIAL_NUM MAC_ADDRESS)

	if(SERIAL_NUM)
		set(SERIAL_NUM_SWITCH "--snr")
	endif()

	execute_process(
		COMMAND nrfjprog -f nrf52 --memrd 0x100000A4 --n 8 ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}
		RESULT_VARIABLE status
		OUTPUT_VARIABLE output
		ERROR_VARIABLE error
		)
	if("${status}" STREQUAL "41")
		message(WARNING "Cannot find a board attached")
		return()
	endif()

	string(REGEX MATCH "^0x([0-9a-fA-F]+): *([0-9a-fA-F]+) *([0-9a-fA-F]+)" Tmp ${output})
	set(Address ${CMAKE_MATCH_1})
	set(Half1 ${CMAKE_MATCH_2})
	set(ValueA ${CMAKE_MATCH_3})

	# The address will be a Random Static Address, denoted by the last bits being 11, hence, the OR with 0xC
	# at exactly the last two bits (endianness) that belong to the 48 bits of the MAC address.
	bitwise_or(${ValueA} "0xC000" ValueB)
	string(TOUPPER "${ValueB}" Half2)

	# Add colons
	words(${Half1} DelimValue1 0 ":")
	words(${Half2} DelimValue2 6 ":")

	message(STATUS "Address: ${DelimValue2}:${DelimValue1}")

	set(${MAC_ADDRESS} "${DelimValue2}:${DelimValue1}" PARENT_SCOPE)
endfunction()
