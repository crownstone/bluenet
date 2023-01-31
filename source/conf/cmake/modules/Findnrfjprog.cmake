# - Try to find nrfjprog
#
# Once done this will define
#  NRFJPROG_FOUND - System has nrfjprog
#  NRFJPROG_EXECUTABLE - The nrfjprog executable

# Check if utility has already been found before and is present in the cache
# (We assume that it has not been removed since then.)
#if(NRFJPROG_FOUND)
#	return()
#endif()

# We won't be using pkg-config for binaries
find_program(NRFJPROG_EXECUTABLE NAMES nrfjprog)

# If XXX_EXECUTABLE is set, also set e.g. XXX_FOUND
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(nrfjprog DEFAULT_MSG NRFJPROG_EXECUTABLE)

if(NOT NRFJPROG_FOUND)
	return()
endif()

# Get version information.
# This assumes we can run nrfjprog --version and it's giving the same output on different operating systems.
execute_process(COMMAND nrfjprog --version RESULT_VARIABLE RESULT OUTPUT_VARIABLE VERSION_UNPARSED)

# Now we have to parse the following type of output:
#   nrfjprog version: 10.14.0 external
#   JLinkARM.dll version: 7.60b

# Replace new lines by semi-colons so we can make a list of it
string(REPLACE "\n" ";" VERSION_UNPARSED_DELIM ${VERSION_UNPARSED})

# Get the first line
list(GET VERSION_UNPARSED_DELIM 0 VERSION_FIRST_LINE)

# Replace colons and spaces by semi-colons
string(REPLACE ":" ";" VERSION_EXTENDED ${VERSION_FIRST_LINE})
string(REPLACE " " ";" VERSION ${VERSION_FIRST_LINE})

# Get version (should be something like 10.14.0)
list(GET VERSION 2 NRFJPROG_VERSION_STRING)

string(REPLACE "." ";" FULL_VERSION ${NRFJPROG_VERSION_STRING})

list(GET FULL_VERSION 0 NRFJPROG_VERSION_MAJOR)
list(GET FULL_VERSION 1 NRFJPROG_VERSION_MINOR)
list(GET FULL_VERSION 2 NRFJPROG_VERSION_PATCH)
