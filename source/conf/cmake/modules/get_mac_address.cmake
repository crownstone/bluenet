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

	message(STATUS "Parse mac address")
	string(REGEX MATCH "^0x([0-9a-fA-F]+): *([0-9a-fA-F]+) *([0-9a-fA-F]+) *([0-9a-fA-F]+) *([0-9a-fA-F]+) *([0-9a-fA-F]+) *([0-9a-fA-F]+)" Tmp ${output})
	set(Byte1 ${CMAKE_MATCH_2})
	set(Byte2 ${CMAKE_MATCH_3})
	set(Byte3 ${CMAKE_MATCH_4})
	set(Byte4 ${CMAKE_MATCH_5})
	set(Byte5 ${CMAKE_MATCH_6})
	set(Byte6 ${CMAKE_MATCH_7})

	# A Bluetooth address consists of 6 bytes (48 bits)
	# It is divided into three parts:
	#  - Two bytes NAP (Non-significant Address Part)
	#  - One byte UAP (Upper Address Part)
	#  - Three bytes LAP (Lower Address Part)
	# The NAP and UAP is also called the OUI (Organizationally Unique Identifier)

	# The OUI has information over the type of address this is.
	# The hardware has a randomly generated address. However, some bits will be overwritten
	# so that it conforms to the Bluetooth spec.

	# The address will be a "Random Static Address"
	#   This is denoted by the last bits being 11, hence, the OR with 0xC0
	#   We only do this for the last two bits (w.r.t. endianness in flash) of the total of 48 bits.
	#   These are the first two bits over the air.
	from_hex("0xC0" Mask)
	from_hex("0x${Byte6}" Byte6Dec)
	bitwise_or(${Byte6Dec} "${Mask}" Byte6StaticAddress)
	to_hex("${Byte6StaticAddress}" Byte6StaticAddressHex "")

	# On the nRF52840 a byte like 0x21 becomes 0xF3. Thus 0010 0001b becomes 1111 0011b.
	# Hence we don't only OR with 0xC0
	# - We OR with 0x01 (multicast rather than unicast)
	# - We OR with 0x02 (locally administered rather than globally enforced)
	# - We OR with 0x10 (TODO: what does this signify?)
	# Total OR becomes 0x13
	if("${NRF_DEVICE}" STREQUAL "NRF52840_XXAA")
		message(STATUS "Apply extra mask for ${NRF_DEVICE} device")
		from_hex("0x13" Mask)
		bitwise_or(${Byte6StaticAddress} "${Mask}" Byte6StaticAddress)
		to_hex("${Byte6StaticAddress}" Byte6StaticAddressHex "")
	else()
		message(STATUS "No extra mask applied for ${NRF_DEVICE} device")
	endif()

	set(Address "${Byte6StaticAddressHex}:${Byte5}:${Byte4}:${Byte3}:${Byte2}:${Byte1}")
	message(STATUS "Bluetooth address: ${Address}")

	set(${MAC_ADDRESS} "${Address}" PARENT_SCOPE)
endfunction()
