# - Try to find nrfjprog
#
# Once done this will define
#  NRFJPROG_FOUND - System has nrfjprog
#  NRFJPROG_EXECUTABLE - The nrfjprog executable

# Check if utility has already been found before and is present in the cache
# (We assume that it has not been removed since then.)
if(NRFJPROG_FOUND)
	return()
endif()

# We won't be using pkg-config for binaries
find_program(NRFJPROG_EXECUTABLE NAMES nrfjprog)

# If XXX_EXECUTABLE is set, also set e.g. XXX_FOUND
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(nrfjprog DEFAULT_MSG NRFJPROG_EXECUTABLE)

