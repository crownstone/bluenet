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

set(gcc_arm_none_eabi_VERSION_STRING "UNKNOWN")

if(NOT gcc_arm_none_eabi_FOUND)
	return()
endif()

# Get version information.
# This assumes we can run JLinKExe with the above script and it's giving the same output on different operating systems.
execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/gcc_arm_none_eabi/bin/arm-none-eabi-gcc --version
	RESULT_VARIABLE RESULT
	OUTPUT_VARIABLE VERSION_UNPARSED)

# Return on empty string or single character (say a new line)
string(LENGTH "${VERSION_UNPARSED}" STR_LENGTH)
if (NOT VERSION_UNPARSED OR STR_LENGTH LESS 2)
	return()
endif()

# Now we have to parse the following type of output:
#
#   arm-none-eabi-gcc (GNU Arm Embedded Toolchain 10.3-2021.10) 10.3.1 20210824 (release)
#   Copyright (C) 2020 Free Software Foundation, Inc.
#   This is free software; see the source for copying conditions.  There is NO
#   warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# Replace new lines by semi-colons so we can make a list of it
string(REPLACE "\n" ";" VERSION_UNPARSED_DELIM ${VERSION_UNPARSED})

# Get the first line
list(GET VERSION_UNPARSED_DELIM 0 VERSION_FIRST_LINE)

# Replace colons and spaces by semi-colons
string(REPLACE ":" ";" VERSION_EXTENDED ${VERSION_FIRST_LINE})
string(REPLACE " " ";" VERSION ${VERSION_FIRST_LINE})

if (LIST_LENGTH LESS 7)
	return()
endif()

unset(gcc_arm_none_eabi_VERSION_STRING)

# Get version (should be something like 10.14.0)
list(GET VERSION 6 gcc_arm_none_eabi_VERSION_STRING)

string(REPLACE "." ";" FULL_VERSION ${gcc_arm_none_eabi_VERSION_STRING})

list(LENGTH FULL_VERSION LIST_LENGTH)

if (LIST_LENGTH LESS 3)
	return()
endif()

list(GET FULL_VERSION 0 gcc_arm_none_eabi_VERSION_MAJOR)
list(GET FULL_VERSION 1 gcc_arm_none_eabi_VERSION_MINOR)
list(GET FULL_VERSION 2 gcc_arm_none_eabi_VERSION_PATCH)
