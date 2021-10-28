if(COMMAND cmake_policy)
	# only interpret arguments as variables when unquoted
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

message(STATUS "Default config file: " ${DEFAULT_CONFIG_FILE})
load_configuration(${DEFAULT_CONFIG_FILE} CONFIG_LIST)

# Overwrite with runtime config
load_configuration(${CONFIG_FILE} CONFIG_LIST)

string(REGEX REPLACE "~" "$ENV{HOME}" KEYS_JSON_FILE "${KEYS_JSON_FILE}")

set(CONFIG_FIELD "$ENV{BLUENET_CONFIG}")

#if(NOT CONFIG_FIELD)
#	message(WARNING "Call with parameter, such as\n BLUENET_CONFIG=MAC_ADDRESS make write_config")
#	return()
#endif()

function(write_field FIELD_NAME FIELD_VALUE)

	set(setupConfigFile "CMakeBuild.dynamic.config")
	set(tmpFile "CMakeBuild.dynamic.config.temp")

	if(EXISTS "${setupConfigFile}")

		# Update a file
		file(READ "${setupConfigFile}" FILE_DATA)

		string(REGEX MATCH "${FIELD_NAME}[ ]*=" FOUND_MATCH "${FILE_DATA}")

		if(FOUND_MATCH)
			if(EXISTS "${tmpFile}")
				file(REMOVE "${tmpFile}")
			endif()
			string(REGEX REPLACE "(${FIELD_NAME}[ ]*=[ ]*)[^\n]+\n" "\\1${FIELD_VALUE}\n" FILE_DATA "${FILE_DATA}")
			file(WRITE "${tmpFile}" "${FILE_DATA}")

			# Check if file is different, then copy over to old file
			execute_process(
				COMMAND ${CMAKE_COMMAND} -E compare_files "${setupConfigFile}" "${tmpFile}"
				RESULT_VARIABLE status
				OUTPUT_VARIABLE output
				ERROR_VARIABLE error
				)
			if(status EQUAL 0)
				message(STATUS "No update required.")
			elseif(status EQUAL 1)
				message(STATUS "Update file ${setupConfigFile}")
				configure_file("${tmpFile}" "${setupConfigFile}" COPYONLY)
			else()
				message(STATUS "Error while comparing the files.")
			endif()
			file(REMOVE "${tmpFile}")

		else()
			message(STATUS "Field not found, write value")
			set(FILE_DATA "${FIELD_NAME} = ${FIELD_VALUE}\n")
			#message(STATUS "Write ${FILE_DATA} to ${setupConfigFile}")
			file(APPEND "${setupConfigFile}" "${FILE_DATA}")
		endif()

	else()
		message(STATUS "Write new ${setupConfigFile} file")
		set(FILE_DATA "${FIELD_NAME} = ${FIELD_VALUE}\n")
		file(WRITE "${setupConfigFile}" "${FILE_DATA}")
	endif()

endfunction()

if(NOT SERIAL_NUM)
	set(SERIAL_NUM "")
endif()

if(NOT MAC_ADDRESS)
	set(MAC_ADDRESS "")
endif()

if(NOT CONFIG_FIELD)
	message(STATUS "Write all fields")
	get_mac_address("${SERIAL_NUM}" MAC_ADDRESS)
	write_field("MAC_ADDRESS" ${MAC_ADDRESS})
	write_field("BEACON_MAJOR" ${BEACON_MAJOR})
	write_field("BEACON_MINOR" ${BEACON_MINOR})
	write_field("BEACON_UUID" ${BEACON_UUID})
	write_field("CROWNSTONE_ID" ${CROWNSTONE_ID})
	write_field("SPHERE_ID" ${SPHERE_ID})
	if(NOT DEFINED MESH_DEVICE_KEY)
		set(MESH_DEVICE_KEY "L7p9byJnriKOECqQ")
	endif()
	write_field("MESH_DEVICE_KEY" ${MESH_DEVICE_KEY})
endif()

# The following ones can also be written individually through e.g.:
#     BLUENET_CONFIG=MAC_ADDRESS make write_config

if(CONFIG_FIELD STREQUAL "MAC_ADDRESS")
	message(STATUS "Get mac address")
	get_mac_address("${SERIAL_NUM}" MAC_ADDRESS)
	write_field("MAC_ADDRESS" ${MAC_ADDRESS})

elseif(CONFIG_FIELD STREQUAL "BEACON_MAJOR")
	message(STATUS "Beacon major: ${BEACON_MAJOR}")
	write_field("BEACON_MAJOR" ${BEACON_MAJOR})

elseif(CONFIG_FIELD STREQUAL "BEACON_MINOR")
	message(STATUS "Beacon minor: ${BEACON_MINOR}")
	write_field("BEACON_MINOR" ${BEACON_MINOR})

elseif(CONFIG_FIELD STREQUAL "BEACON_UUID")
	message(STATUS "Beacon uuid: ${BEACON_UUID}")
	write_field("BEACON_UUID" ${BEACON_UUID})

elseif(CONFIG_FIELD STREQUAL "CROWNSTONE_ID")
	message(STATUS "Crownstone id: ${CROWNSTONE_ID}")
	write_field("CROWNSTONE_ID" ${CROWNSTONE_ID})

elseif(CONFIG_FIELD STREQUAL "SPHERE_ID")
	message(STATUS "Sphere id: ${SPHERE_ID}")
	write_field("SPHERE_ID" ${SPHERE_ID})

elseif(CONFIG_FIELD STREQUAL "MESH_DEVICE_KEY")
	if(NOT DEFINED MESH_DEVICE_KEY)
		# Not defined, can be set to random string, but that would write all the time something else
		# string(RANDOM LENGTH 16 MESH_DEVICE_KEY)
		set(MESH_DEVICE_KEY "L7p9byJnriKOECqQ")
	endif()
	message(STATUS "Mesh device key: ${MESH_DEVICE_KEY}")
	write_field("MESH_DEVICE_KEY" ${MESH_DEVICE_KEY})


endif()
