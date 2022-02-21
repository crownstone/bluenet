if(COMMAND cmake_policy)
	# only interpret arguments as variables when unquoted
	cmake_policy(SET CMP0054 NEW)
endif()

include(${DEFAULT_MODULES_PATH}/load_configuration.cmake)

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
		message(STATUS "Load from default config file: " ${DEFAULT_CONFIG_FILE})
		load_configuration(${DEFAULT_CONFIG_FILE} CONFIG_LIST)
	endif()
endif()

# Load target file
if(DEFINED TARGET_CONFIG_FILE)
	if(EXISTS "${TARGET_CONFIG_FILE}")
		message(STATUS "Load from target config file: " ${TARGET_CONFIG_FILE})
		load_configuration(${TARGET_CONFIG_FILE} CONFIG_LIST)
	endif()
endif()

# Load target overwrite file
if(DEFINED TARGET_CONFIG_OVERWRITE_FILE)
	if(EXISTS ${TARGET_CONFIG_OVERWRITE_FILE})
		message(STATUS "Load from target overwrite file: " ${TARGET_CONFIG_OVERWRITE_FILE})
		load_configuration(${TARGET_CONFIG_OVERWRITE_FILE} CONFIG_LIST)
	endif()
endif()

# Overwrite with runtime config
if(DEFINED CONFIG_FILE)
	if(EXISTS "${CONFIG_FILE}")
		message(STATUS "Load from runtime config file: " ${CONFIG_FILE})
		load_configuration(${CONFIG_FILE} CONFIG_LIST)
	else()
		message(STATUS "Note, there is no runtime config file...")
	endif()
endif()

set(TEMPLATE_DIR ${DEFAULT_MODULES_PATH}/../../../../scripts/templates)
message(STATUS "Generate log-client.sh script")
configure_file(${TEMPLATE_DIR}/log-client.sh.in tmp/log-client.sh)
file(COPY tmp/log-client.sh DESTINATION . FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)

set(CONF_DIR ${DEFAULT_MODULES_PATH}/../..)
message(STATUS "Generate debug-application.sh script")
configure_file(${TEMPLATE_DIR}/debug-application.sh.in tmp/debug-application.sh)
file(COPY tmp/debug-application.sh DESTINATION . FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)

set(CONF_DIR ${DEFAULT_MODULES_PATH}/../..)
message(STATUS "Generate debug-bootloader.sh script")
configure_file(${TEMPLATE_DIR}/debug-bootloader.sh.in tmp/debug-bootloader.sh)
file(COPY tmp/debug-bootloader.sh DESTINATION . FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)

