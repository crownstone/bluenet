# - Try to find LibXml2
# Once done this will define
#  PASS_FOUND - System has pass
#  PASS_EXECUTABLE - The pass executable

find_package(PkgConfig)
pkg_check_modules(PASS QUIET pass)

if(PASS_EXECUTABLE)
	# in cache already
	set(PASS_FOUND TRUE)
else()
	find_program(PASS_EXECUTABLE NAMES pass)
endif()

