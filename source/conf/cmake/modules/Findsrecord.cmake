# - Try to find LibXml2
# Once done this will define
#  SRECORD_FOUND - System has pass
#  SRECORD_EXECUTABLE - The pass executable

find_package(PkgConfig)
pkg_check_modules(SRECORD QUIET srecord)

if(SRECORD_EXECUTABLE)
	# in cache already
	set(SRECORD_FOUND TRUE)
else()
	find_program(SRECORD_EXECUTABLE NAMES srec_cat)
endif()

