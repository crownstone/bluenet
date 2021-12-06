# - Try to find srecord

# Once done this will define
#  SRECORD_FOUND      - System has srecord
#  SRECORD_EXECUTABLE - The srecord executable

find_package(PkgConfig)
pkg_check_modules(SRECORD QUIET srecord)

if(SRECORD_EXECUTABLE)
	# in cache already
	set(SRECORD_FOUND TRUE)
else()
	find_program(SRECORD_EXECUTABLE NAMES srec_cat)
endif()

