include(${DEFAULT_MODULES_PATH}/hex.cmake)
include(${DEFAULT_MODULES_PATH}/load_hardware_version_mapping.cmake)
include(${DEFAULT_MODULES_PATH}/load_configuration.cmake)

# Usage:
#   cmake <CONFIG_FILE> <NRF_DEVICE_FAMILY> <INSTRUCTION>
#
# Examples of instructions:
#   cmake ... READ <ADDRESS> [COUNT] [SERIAL_NUM]
#   cmake ... LIST [SERIAL_NUM]
#   cmake ... RESET [SERIAL_NUM]
#   cmake ... ERASE [SERIAL_NUM]

set(DRY_RUN 0)

if(EXISTS "${DEFAULT_CONFIG_FILE}")
	load_configuration(${DEFAULT_CONFIG_FILE} CONFIG_LIST)
endif()

# Overwrite with runtime config
if(EXISTS "${CONFIG_FILE}")
	load_configuration(${CONFIG_FILE} CONFIG_LIST)
endif()

if(SERIAL_NUM)
	message("Add serial num flag for ${SERIAL_NUM}")
	set(SERIAL_NUM_SWITCH "--snr")
endif()

if(COUNT)
	message("Add count flag ${COUNT}")
	set(COUNT_SWITCH "--n")
endif()

if(DRY_RUN)
	message(STATUS "Dry run")
endif()

if(INSTRUCTION STREQUAL "READ")
	if(DRY_RUN)
		message(STATUS "$> nrfjprog -f nrf52 --memrd ${ADDRESS} ${COUNT_SWITCH} ${COUNT} ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}")
		return()
	endif()
	execute_process(
		COMMAND nrfjprog -f nrf52 --memrd ${ADDRESS} ${COUNT_SWITCH} ${COUNT} ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}
		RESULT_VARIABLE status
		OUTPUT_VARIABLE output
		ERROR_VARIABLE error
		)
	if("${status}" STREQUAL "40")
		message(FATAL_ERROR "Mmmm..... You sure you added the RIGHT(!) JTAG debugger? It should be the one with serial number ${SERIAL_NUM}.")
	endif()
	if("${status}" STREQUAL "41")
		message(FATAL_ERROR "Mmmm..... You sure you added the (right) JTAG debugger?")
	endif()
	# success
	if("${status}" STREQUAL "0")
		# softdevice check
		if ("${ADDRESS}" STREQUAL "0x000300C")
			string(REGEX MATCH "^0x([0-9a-fA-F]+): *([0-9a-fA-F]+)" Tmp ${output})
			set(Address ${CMAKE_MATCH_1})
			set(Value ${CMAKE_MATCH_2})
			#message(STATUS "${Value}")
			if ("${Value}" STREQUAL "FFFFFFFF")
				message(STATUS "There has been no SoftDevice uploaded to this board")
			elseif ("${Value}" STREQUAL "FFFF00A8")
				message(STATUS "Softdevice S132 6.0.0")
			elseif ("${Value}" STREQUAL "FFFF00AF")
				message(STATUS "Softdevice S132 6.1.0")
			elseif ("${Value}" STREQUAL "FFFF00B7")
				message(STATUS "Softdevice S132 6.1.1")
			elseif ("${Value}" STREQUAL "FFFF00CB")
				message(STATUS "Softdevice S132 7.0.1")
			elseif ("${Value}" STREQUAL "FFFF0101")
				message(STATUS "Softdevice S132 7.2.0")
			else()
				message(STATUS "Unknown softdevice (${Value})!!")
			endif()
		elseif ("${ADDRESS}" STREQUAL "0x10000100") # NRF chip version: Part
			string(REGEX MATCH "^0x([0-9a-fA-F]+): *([0-9a-fA-F]+)" Tmp ${output})
			set(Address ${CMAKE_MATCH_1})
			set(Value ${CMAKE_MATCH_2})
			#message(STATUS "Value read: ${Value}")
			if ("${Value}" STREQUAL "00052832")
				message(STATUS "Part: nRF52832")
			else()
				message(STATUS "Unknown chipset")
			endif()
		elseif ("${ADDRESS}" STREQUAL "0x10000104") # NRF chip version: Variant
			string(REGEX MATCH "^0x([0-9a-fA-F]+): *([0-9a-fA-F]+)" Tmp ${output})
			set(Address ${CMAKE_MATCH_1})
			set(Value ${CMAKE_MATCH_2})
			#message(STATUS "Value read : ${Value}")
			# Should actually use conversion to ASCII
			if ("${Value}" STREQUAL "41414141")
				message(STATUS "Variant: AAAA")
			elseif ("${Value}" STREQUAL "41414142")
				message(STATUS "Variant: AAAB")
			elseif ("${Value}" STREQUAL "41414241")
				message(STATUS "Variant: AABA")
			elseif ("${Value}" STREQUAL "41414242")
				message(STATUS "Variant: AABB")
			elseif ("${Value}" STREQUAL "41414230")
				message(STATUS "Variant: AAB0")
			elseif ("${Value}" STREQUAL "41414530")
				message(STATUS "Variant: AAE0")
			elseif ("${Value}" STREQUAL "41414630")
				message(STATUS "Variant: AAF0")
			endif()
		elseif ("${ADDRESS}" STREQUAL "0x10001014") # Bootloader address in UICR
			string(REGEX MATCH "^0x([0-9a-fA-F]+): *([0-9a-fA-F]+)" Tmp ${output})
			set(Address ${CMAKE_MATCH_1})
			set(Value ${CMAKE_MATCH_2})
			if ("${Value}" STREQUAL "FFFFFFFF")
				message(STATUS "Bootloader might be present, but UICR.BOOTLOADERADDR (0x10001014) seems not to be set!")
			else()
				message(STATUS "Bootloader address starts at ${Value} according to UICR->NRFFW[0]")
			endif()
		elseif ("${ADDRESS}" STREQUAL "0x10001018") # MBR parameter page address in UICR
			string(REGEX MATCH "^0x([0-9a-fA-F]+): *([0-9a-fA-F]+)" Tmp ${output})
			set(Address ${CMAKE_MATCH_1})
			set(Value ${CMAKE_MATCH_2})
			if ("${Value}" STREQUAL "FFFFFFFF")
				message(STATUS "No MBR parameter page address set in UICR!")
			else()
				message(STATUS "MBR parameter page address starts at ${Value} according to UICR->NRFFW[1]")
			endif()
		elseif ("${ADDRESS}" STREQUAL "0x10001084") # Board version
			string(REGEX MATCH "^0x([0-9a-fA-F]+): *([0-9a-fA-F]+)" Tmp ${output})
			set(Address ${CMAKE_MATCH_1})
			set(Value ${CMAKE_MATCH_2})
			from_hex(${Value} DecimalValue)
			# Newer version of CMAKE does have a hex to dec conversion
			# math(EXPR DecimalValue "0x${Value}" OUTPUT_FORMAT DECIMAL)
			set(BOARD_VERSION ${DecimalValue})
			# TODO: Should get path in other fashion
			load_board_name(${DEFAULT_MODULES_PATH}/../../../include/cfg/cs_Boards.h BOARD_NAME ${BOARD_VERSION})
			message(STATUS "Board version: ${BOARD_VERSION}")
			message(STATUS "Board name: ${BOARD_NAME}")
		elseif ("${ADDRESS}" STREQUAL "0x100000A4") # MAC address
			string(REGEX MATCH "^0x([0-9a-fA-F]+): *([0-9a-fA-F]+) *([0-9a-fA-F]+)" Tmp ${output})
			set(Address ${CMAKE_MATCH_1})
			set(Half1 ${CMAKE_MATCH_2})
			set(ValueA ${CMAKE_MATCH_3})
			# The address will be a Random Static Address, denoted by the last bits being 11, hence, the OR with 0xC
			# at exactly the last two bits (endianness) that belong to the 48 bits of the MAC address.
			bitwise_or_hex(${ValueA} "0xC000" ValueB)
			string(TOUPPER "${ValueB}" Half2)
			words(${Half1} DelimValue1 0 ":")
			words(${Half2} DelimValue2 6 ":")
			message(STATUS "Address: ${DelimValue2}:${DelimValue1}")
		else()
			message(STATUS "Unknown address to parse: ${ADDRESS}")
			message(STATUS "Result: \n\n${output}")
		endif()
	endif()

elseif(INSTRUCTION STREQUAL "WRITE")
	message(STATUS "Write value ${VALUE} to address ${ADDRESS}")
	message(STATUS "$> nrfjprog -f ${NRF_DEVICE_FAMILY} --memwr ${ADDRESS} --val ${VALUE} ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}")
	if(DRY_RUN)
		return()
	endif()
	execute_process(
		COMMAND nrfjprog -f ${NRF_DEVICE_FAMILY} --memwr ${ADDRESS} --val ${VALUE} ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}
		RESULT_VARIABLE status
		OUTPUT_VARIABLE output
		ERROR_VARIABLE error
		)
	message(STATUS "Output \n\n${output}")
	if (error)
		message(STATUS "Error ${error}")
	endif()
	if (status)
		message(STATUS "Status ${status}")
	endif()

elseif(INSTRUCTION STREQUAL "LIST")
	if (SERIAL_NUM)
		message(STATUS "Currently configured serial num: ${SERIAL_NUM}")
	endif()
	execute_process(
		COMMAND nrfjprog --ids
		RESULT_VARIABLE status
		OUTPUT_VARIABLE output
		ERROR_VARIABLE error
		)
	#message(STATUS "Status ${status}")
	if (output)
		message(STATUS "List attached debuggers: \n\n ${output}")
	endif()
	if (error)
		message(STATUS "Error ${error}")
	endif()

elseif(INSTRUCTION STREQUAL "WRITE_BINARY")
	set(ERASE_OPTION "--sectorerase")
	# chiperase includes UICR (but cannot be used in a sequence for bootloader, softdevice, and firmware)
	#set(ERASE_OPTION "--chiperase") 
	message(STATUS "Flashing ${BINARY} with erase option ${ERASE_OPTION}")
	message(STATUS "$> nrfjprog -f ${NRF_DEVICE_FAMILY} --program ${BINARY} ${ERASE_OPTION} ${SERIAL_NUM_SWITCH} ${SERIAL_NUM} --verify")
	if(DRY_RUN)
		return()
	endif()
	execute_process(
		COMMAND nrfjprog -f ${NRF_DEVICE_FAMILY} --program ${BINARY} ${ERASE_OPTION} ${SERIAL_NUM_SWITCH} ${SERIAL_NUM} --verify
		RESULT_VARIABLE status
		OUTPUT_VARIABLE output
		ERROR_VARIABLE error
		)
	message(STATUS "Output listing \n\n${output}")
	if (error)
		message(STATUS "Error ${error}")
	endif()
	if (status)
		message(STATUS "Status ${status}")
	endif()

elseif(INSTRUCTION STREQUAL "RESET")
	message(STATUS "$> nrfjprog -f ${NRF_DEVICE_FAMILY} --reset ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}")
	if(DRY_RUN)
		return()
	endif()
	execute_process(
		COMMAND nrfjprog -f ${NRF_DEVICE_FAMILY} --reset ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}
		RESULT_VARIABLE status
		OUTPUT_VARIABLE output
		ERROR_VARIABLE error
		)
	message(STATUS "Output \n\n${output}")
	if (error)
		message(STATUS "Error ${error}")
	endif()

elseif(INSTRUCTION STREQUAL "ERASE")
	message(STATUS "$> nrfjprog -f nrf52 --eraseall ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}")
	if(DRY_RUN)
		return()
	endif()
	execute_process(
		COMMAND nrfjprog -f nrf52 --eraseall ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}
		RESULT_VARIABLE status
		OUTPUT_VARIABLE output
		ERROR_VARIABLE error
		)
	message(STATUS "Output \n\n${output}")
	if (error)
		message(STATUS "Error ${error}")
	endif()

endif()

