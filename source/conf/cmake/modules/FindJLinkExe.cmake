# - Try to find JLinkExe
#
# Once done this will define
#  JLINKEXE_FOUND - System has JLinkExe
#  JLINKEXE_EXECUTABLE - The JLinkExe executable

# Check if utility has already been found before and is present in the cache
# (We assume that it has not been removed since then.)
if(JLINKEXE_FOUND)
	return()
endif()

# We won't be using pkg-config for binaries
find_program(JLINKEXE_EXECUTABLE NAMES JLinkExe)

# If XXX_EXECUTABLE is set, also set e.g. XXX_FOUND
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JLinkExe DEFAULT_MSG JLINKEXE_EXECUTABLE)

if(NOT JLINKEXE_FOUND)
	return()
endif()

# Get version information.
# This assumes we can run JLinKExe with the above script and it's giving the same output on different operating systems.
# TODO: Note that on Windows the name of the executable is JLink.exe
execute_process(COMMAND JLinkExe -CommandFile ${CMAKE_CURRENT_SOURCE_DIR}/source/conf/jlink/nop.jlink RESULT_VARIABLE RESULT OUTPUT_VARIABLE VERSION_UNPARSED)

# Now we have to parse the following type of output:
#
#   SEGGER J-Link Commander V7.60b (Compiled Dec 22 2021 14:45:57)
#   DLL version V7.60b, compiled Dec 22 2021 12:54:38
#
#   J-Link Command File read successfully.
#   Processing script file...
#   J-Link>q
#
#   Script processing completed.

# Replace new lines by semi-colons so we can make a list of it
string(REPLACE "\n" ";" VERSION_UNPARSED_DELIM ${VERSION_UNPARSED})

# Get the first line
list(GET VERSION_UNPARSED_DELIM 0 VERSION_FIRST_LINE)

# Replace colons and spaces by semi-colons
string(REPLACE ":" ";" VERSION_EXTENDED ${VERSION_FIRST_LINE})
string(REPLACE " " ";" VERSION ${VERSION_FIRST_LINE})

# Get version (should be something like V7.60b)
list(GET VERSION 3 JLINK_VERSION_STRING)

string(REPLACE "." ";" FULL_VERSION ${JLINK_VERSION_STRING})

list(GET FULL_VERSION 0 JLINK_VERSION_MAJOR)
list(GET FULL_VERSION 1 JLINK_VERSION_MINOR)
