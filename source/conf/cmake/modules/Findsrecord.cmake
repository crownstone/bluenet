# - Try to find srecord

# Once done this will define
#  SRECORD_FOUND      - System has srecord
#  SRECORD_EXECUTABLE - The srecord executable

# Check if utility has already been found before and is present in the cache
# (We assume that it has not been removed since then.)
if(SRECORD_FOUND)
	return()
endif()

# We won't be using pkg-config for binaries
find_program(SRECORD_EXECUTABLE NAMES srec_cat)

# If XXX_EXECUTABLE is set, also set e.g. XXX_FOUND
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(srecord DEFAULT_MSG SRECORD_EXECUTABLE)

