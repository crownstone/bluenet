# Try to find libudev-dev
#
# Once done this will define
#  UDEV_FOUND        - System has udev
#  UDEV_LIBRARIES    - Library itself
#  UDEV_INCLUDE_DIRS - Dirs to include

# Check if udev has already been found before and is present in the cache
# (We assume that it has not been removed since then.)
if(UDEV_LIBRARY)
	set(UDEV_FOUND TRUE)
	return()
endif()

# Use pkg_config to find the library
find_package(PkgConfig)
pkg_check_modules(PKG_LIBUDEV QUIET libudev)

# Search for libudev.h (and give pkg-config output as hints)
find_path(UDEV_INCLUDE_DIR NAMES libudev.h
	HINTS
	${PKG_LIBUDEV_INCLUDE_DIRS}
	${PKG_LIBUDEV_INCLUDEDIR}
	)

# Search for libudev.so (and give pkg-config output as hints)
find_library(UDEV_LIBRARY NAMES udev
	HINTS
	${PKG_LIBUDEV_LIBDIR}
	${PKG_LIBUDEV_LIBRARY_DIRS}
	)

set(UDEV_INCLUDE_DIRS ${UDEV_INCLUDE_DIR})
set(UDEV_LIBRARIES ${UDEV_LIBRARY})
