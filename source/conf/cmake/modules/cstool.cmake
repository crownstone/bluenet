if(COMMAND cmake_policy)
	cmake_policy(SET CMP0054 NEW)
endif()

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

# Load default config
if(DEFINED DEFAULT_CONFIG_FILE)
	if(EXISTS "${DEFAULT_CONFIG_FILE}")
		load_configuration(${DEFAULT_CONFIG_FILE} CONFIG_DUMMY_LIST)
	endif()
endif()

# Overwrite with runtime config
if(DEFINED CONFIG_FILE)
	if(EXISTS "${CONFIG_FILE}")
		load_configuration(${CONFIG_FILE} CONFIG_DUMMY_LIST)
	endif()
endif()

if(DEFINED TARGET_CONFIG_FILE)
	if(EXISTS "${TARGET_CONFIG_FILE}")
		load_configuration(${TARGET_CONFIG_FILE} CONFIG_DUMMY_LIST)
	endif()
endif()

if(DEFINED TARGET_CONFIG_OVERWRITE_FILE)
	if(EXISTS ${TARGET_CONFIG_OVERWRITE_FILE})
		load_configuration(${TARGET_CONFIG_OVERWRITE_FILE} CONFIG_DUMMY_LIST)
	endif()
endif()

# Get MAC address etc.
set(setupConfigFile "CMakeBuild.dynamic.config")
if(EXISTS "${setupConfigFile}")
	load_configuration(${setupConfigFile} CONFIG_DUMMY_LIST)
else()
	message(FATAL_ERROR "${setupConfigFile} does not exist. First run make write_config or fill file manually.")
endif()

message(STATUS "Connect to device with address ${MAC_ADDRESS}")

set(setupConfigJsonFile "config.json")

string(REGEX REPLACE "~" "$ENV{HOME}" KEYS_JSON_FILE "${KEYS_JSON_FILE}")

if(INSTRUCTION STREQUAL "SETUP")
	message(STATUS "Set up Crownstone (timeout is 60 seconds)")
	file(WRITE ${setupConfigJsonFile} "{\n")
	file(APPEND ${setupConfigJsonFile} "  \"crownstoneId\": ${CROWNSTONE_ID},\n")
	file(APPEND ${setupConfigJsonFile} "  \"sphereId\": ${SPHERE_ID},\n")
	file(APPEND ${setupConfigJsonFile} "  \"meshDeviceKey\": \"${MESH_DEVICE_KEY}\",\n")
	file(APPEND ${setupConfigJsonFile} "  \"ibeaconUUID\": \"${BEACON_UUID}\",\n")
	file(APPEND ${setupConfigJsonFile} "  \"ibeaconMajor\": ${BEACON_MAJOR},\n")
	file(APPEND ${setupConfigJsonFile} "  \"ibeaconMinor\": ${BEACON_MINOR}\n")
	file(APPEND ${setupConfigJsonFile} "}\n")
	file(READ ${setupConfigJsonFile} FILE_CONTENTS)
	message(STATUS "The meshDeviceKey should be a random string (different for each device): ${MESH_DEVICE_KEY}")
	message(STATUS "The sphere ID should be a number (same for each Crownstone in the sphere, it is the uid field in the cloud): ${SPHERE_ID}")
	message(STATUS "The Crownstone ID should be a number (different for each Crownstone in the sphere): ${CROWNSTONE_ID}")
	if(CROWNSTONE_ID STREQUAL "0")
		message(WARNING "Incorrect stone id, adjust values in ${setupConfigFile}")
		return()
	endif()
	if(SPHERE_ID STREQUAL "0")
		message(WARNING "Incorrect sphere id, adjust values in ${setupConfigFile}")
		return()
	endif()
	message(STATUS "csutil \"${KEYS_JSON_FILE}\" \"setup\" \"${MAC_ADDRESS}\" \"${setupConfigJsonFile}\"")
	execute_process(
		COMMAND ${DEFAULT_MODULES_PATH}/../../../../tools/csutil/csutil "${KEYS_JSON_FILE}" "setup" "${MAC_ADDRESS}" "${setupConfigJsonFile}"
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
		message(STATUS "+ Are you e.g. sure that you ran cmake with -DDOWNLOAD_CSUTIL?")
		message(STATUS "+ Did you set KEYS_JSON_FILE to a file with keys?")
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

