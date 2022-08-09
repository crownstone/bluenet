# - Try to find gcc_arm_none_eabi
#
# Once done this will define
#  gcc_arm_none_eabi_FOUND - System has gcc_arm_none_eabi
#  gcc_arm_none_eabi_EXECUTABLE - The gcc_arm_none_eabi executable

# Check if utility has already been found before and is present in the cache
# (We assume that it has not been removed since then.)
if(gcc_arm_none_eabi_FOUND)
	return()
endif()

# We won't be using pkg-config for binaries
find_program(gcc_arm_none_eabi_EXECUTABLE NAMES arm-none-eabi-gcc
	PATHS tools/gcc_arm_none_eabi/bin)

# If XXX_EXECUTABLE is set, also set e.g. XXX_FOUND
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(gcc_arm_none_eabi DEFAULT_MSG gcc_arm_none_eabi_EXECUTABLE)

