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

