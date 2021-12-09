# - Try to find pass
#
# Once done this will define
#  PASS_FOUND - System has pass
#  PASS_EXECUTABLE - The pass executable

# Check if utility has already been found before and is present in the cache
# (We assume that it has not been removed since then.)
if(PASS_FOUND)
	return()
endif()

# We won't be using pkg-config for binaries
find_program(PASS_EXECUTABLE NAMES pass)

# If XXX_EXECUTABLE is set, also set e.g. XXX_FOUND
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(pass DEFAULT_MSG PASS_EXECUTABLE)

