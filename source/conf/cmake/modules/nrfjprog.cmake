include(${DEFAULT_MODULES_PATH}/hex.cmake)
include(${DEFAULT_MODULES_PATH}/load_hardware_version_mapping.cmake)

if(SERIAL_NUM) 
  set(SERIAL_NUM_SWITCH "--snr")	
endif()

if(INSTRUCTION STREQUAL "READ")
  execute_process(
    COMMAND nrfjprog -f nrf52 --memrd ${ADDRESS} ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}
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
      else()
	message(STATUS "Unknown softdevice!!")
      endif() 
    elseif ("${ADDRESS}" STREQUAL "0x10000100") # Board version
      string(REGEX MATCH "^0x([0-9a-fA-F]+): *([0-9a-fA-F]+)" Tmp ${output})
      set(Address ${CMAKE_MATCH_1})
      set(Value ${CMAKE_MATCH_2})
      #message(STATUS "Value read: ${Value}")
      if ("${Value}" STREQUAL "00052832") 
	message(STATUS "Hardware version, the nRF52832 chipset")
      else()
	message(STATUS "Unknow chipset")
      endif() 
    elseif ("${ADDRESS}" STREQUAL "0x10000104") # Board, part variant
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
      endif()
    elseif ("${ADDRESS}" STREQUAL "0x10001014") # Bootloader address
      string(REGEX MATCH "^0x([0-9a-fA-F]+): *([0-9a-fA-F]+)" Tmp ${output})
      set(Address ${CMAKE_MATCH_1})
      set(Value ${CMAKE_MATCH_2})
      if ("${Value}" STREQUAL "FFFFFFFF") 
	message(STATUS "Bootloader might be present, but UICR.BOOTLOADERADDR (0x10001014) seems not to be set!")
      else()
	message(STATUS "Bootloader address starts at ${Value}")
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
    else()
      message(STATUS "Unknown address to check: ${ADDRESS}")
    endif()
  endif()

  #message(STATUS "Status ${status}")
  #message(STATUS "Output ${output}")
  #message(STATUS "Error ${error}")
elseif(INSTRUCTION STREQUAL "WRITE")
  message(FATAL_ERROR "Needs to be implemented (argument needs to be written needs to be passed")
  execute_process(
    COMMAND echo nrfjprog -f nrf52 --memwr ${ADDRESS} ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}
    COMMAND nrfjprog -f nrf52 --memwr ${ADDRESS} ${SERIAL_NUM_SWITCH} ${SERIAL_NUM}
    RESULT_VARIABLE status
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
    )
  #message(STATUS "Status ${status}")
  message(STATUS "Output ${output}")
  message(STATUS "Error ${error}")
endif()

