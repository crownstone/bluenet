include(${DEFAULT_MODULES_PATH}/hex.cmake)

function(get_mac_address SERIAL_NUM MAC_ADDRESS)

	if(SERIAL_NUM)
		set(SERIAL_NUM_SWITCH "--snr")
	endif()

	execute_process(
		COMMAND nrfjprog -f nrf52 --memrd 0x100000A4 --w 8 --n 6 ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}
		RESULT_VARIABLE status
		OUTPUT_VARIABLE output
		ERROR_VARIABLE error
		)
	if("${status}" STREQUAL "41")
		message(WARNING "Cannot find a board attached")
		return()
	endif()

	message(STATUS "Parse mac address output \"${output}\"")
	string(REGEX MATCH "^0x([0-9a-fA-F]+): *([0-9a-fA-F]+) *([0-9a-fA-F]+) *([0-9a-fA-F]+) *([0-9a-fA-F]+) *([0-9a-fA-F]+) *([0-9a-fA-F]+)" Tmp ${output})
	set(Byte1 ${CMAKE_MATCH_2})
	set(Byte2 ${CMAKE_MATCH_3})
	set(Byte3 ${CMAKE_MATCH_4})
	set(Byte4 ${CMAKE_MATCH_5})
	set(Byte5 ${CMAKE_MATCH_6})
	set(Byte6 ${CMAKE_MATCH_7})

	# The address will be a Random Static Address
	# This is denoted by the last bits being 11, hence, the OR with 0xC0
	# We only do this for the last two bits (w.r.t. endianness in flash) of the total of 48 bits.
	# These are the first two bits over the air
	from_hex("0xC0" Mask)
	bitwise_or(${Byte6} "${Mask}" Byte6StaticAddress)
	to_hex("${Byte6StaticAddress}" Byte6StaticAddressHex "")

	set(Address "${Byte6StaticAddressHex}:${Byte5}:${Byte4}:${Byte3}:${Byte2}:${Byte1}")
	message(STATUS "Address: ${Address}")

	set(${MAC_ADDRESS} "${Address}" PARENT_SCOPE)
endfunction()
