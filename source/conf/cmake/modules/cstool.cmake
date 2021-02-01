include(${DEFAULT_MODULES_PATH}/load_configuration.cmake)
include(${DEFAULT_MODULES_PATH}/get_mac_address.cmake)

# Usage:
#   cmake <CONFIG_FILE> <NRF_DEVICE_FAMILY> <INSTRUCTION>
#
# Examples of instructions:
#   cmake ... READ <ADDRESS> [COUNT] [SERIAL_NUM]
#   cmake ... LIST [SERIAL_NUM]
#   cmake ... RESET [SERIAL_NUM]
#   cmake ... ERASE [SERIAL_NUM]

# Overwrite with runtime config
load_configuration(${CONFIG_FILE} CONFIG_LIST)

#get_mac_address(${SERIAL_NUM} MAC_ADDRESS)
set(MAC_ADDRESS "C2:81:75:46:D8:F1")

set(setupConfigFile "config.json")

string(REGEX REPLACE "~" "$ENV{HOME}" KEYS_JSON_FILE "${KEYS_JSON_FILE}")

if(INSTRUCTION STREQUAL "SETUP")
	message(STATUS "Set up Crownstone (timeout is 60 seconds)")
	file(WRITE ${setupConfigFile} "{\n")
	file(APPEND ${setupConfigFile} "  \"crownstoneId\": 1,\n")
	file(APPEND ${setupConfigFile} "  \"sphereId\": 1,\n")
	file(APPEND ${setupConfigFile} "  \"meshDeviceKey\": \"IamTheMeshKeyJey\",\n")
	file(APPEND ${setupConfigFile} "  \"ibeaconUUID\": \"1843423e-e175-4af0-a2e4-31e32f729a8a\",\n")
	file(APPEND ${setupConfigFile} "  \"ibeaconMajor\": 123,\n")
	file(APPEND ${setupConfigFile} "  \"ibeaconMinor\": 456\n")
	file(APPEND ${setupConfigFile} "}\n")
	file(READ ${setupConfigFile} FILE_CONTENTS)
	execute_process(
		COMMAND ${DEFAULT_MODULES_PATH}/../../../../tools/csutil/csutil "${KEYS_JSON_FILE}" "setup" "${MAC_ADDRESS}" "${setupConfigFile}"
		TIMEOUT 60
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
elseif(INSTRUCTION STREQUAL "FACTORY_RESET")
	execute_process(
		COMMAND ${DEFAULT_MODULES_PATH}/../../../../tools/csutil/csutil ${KEYS_JSON_FILE} factory_reset ${MAC_ADDRESS} emptyFile
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
endif()

