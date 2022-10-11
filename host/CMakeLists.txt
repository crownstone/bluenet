cmake_minimum_required(VERSION 3.10)

project(bluenet-host)

#set(VERBOSITY 4)

if(NOT VERBOSITY)
	set(VERBOSITY 0)
endif()

include (cs_list_utils)

LOGi("Building bluenet_host project.")
LOGv("---------------------------------------------------")
LOGd("NORDIC_SDK_HOST_TARGET: " ${NORDIC_SDK_HOST_TARGET})
LOGd("NRF5_HOST_DIR: ${NRF5_HOST_DIR}")
LOGd("WORKSPACE_DIR: " ${WORKSPACE_DIR})
LOGd("CMAKE_BUILD_TYPE: " ${CMAKE_BUILD_TYPE})
LOGd("NORDIC_SDK_VERSION_FULL: " ${NORDIC_SDK_VERSION_FULL})
LOGd("SERIAL_VERBOSITY: " ${SERIAL_VERBOSITY})
LOGd("CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
LOGd("DEFAULT_CONFIG_FILE: ${DEFAULT_CONFIG_FILE}")


set(DEFAULT_CXX_FLAGS        "-std=c++17 -Wno-write-strings -fno-exceptions -Wformat=0")
set(DEFAULT_C_FLAGS          "-std=gnu99")
set(DEFAULT_C_AND_CXX_FLAGS  "-g3 -Wall")

set(CMAKE_CXX_FLAGS          "${DEFAULT_CXX_FLAGS}"             CACHE STRING "C++ flags" FORCE)
set(CMAKE_C_FLAGS            "${DEFAULT_C_FLAGS}"               CACHE STRING "C flags" FORCE)
set(CMAKE_C_AND_CXX_FLAGS    "${DEFAULT_C_AND_CXX_FLAGS}"       CACHE STRING "C and C++ flags")

set(CMAKE_BLUENET_MOCK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../mock/source)
set(CMAKE_BLUENET_SOURCE_DIR_MOCK ${CMAKE_BLUENET_MOCK_DIR}/src)
set(CMAKE_BLUENET_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../source)

set(CMAKE_BLUENET_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/bluenet)

add_definitions("-DUSE_APP_CONFIG")
add_compile_definitions(HOST_TARGET)
LOGd("HOST_TARGET: " ${HOST_TARGET})

add_compile_definitions("NRF52832_XXAA")

# add paths to our custom cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_BLUENET_SOURCE_DIR}/conf")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_BLUENET_SOURCE_DIR}/conf/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_BLUENET_SOURCE_DIR}/conf/cmake/modules")
LogListD("CMAKE_MODULE_PATH: " CMAKE_MODULE_PATH)

include(load_configuration)
load_configuration("${DEFAULT_CONFIGURATION_FILE}" CMAKE_BLUENET_ARGS)
load_configuration("${BOARD_TARGET_CONFIGURATION_FILE}" CMAKE_BLUENET_ARGS)
load_configuration("${BOARD_TARGET_OVERWRITE_CONFIGURATION_FILE}" CMAKE_BLUENET_ARGS)

LOGv("*************************************************")

# These should come from a CMakeBuild.config.default file
add_compile_definitions("SERIAL_VERBOSITY=${SERIAL_VERBOSITY}")

####################################
# Auto generated/configuration files
####################################

LOGi("Configure cs_Git.cpp file")
configure_file("${CMAKE_BLUENET_SOURCE_DIR}/src/cfg/cs_Git.cpp.in" "${CMAKE_BLUENET_BINARY_DIR}/src/cfg/cs_Git.cpp")
list(APPEND GENERATED_SOURCES "${CMAKE_BLUENET_BINARY_DIR}/src/cfg/cs_Git.cpp")

LOGi("Configure cs_AutoConfig.cpp file")
configure_file("${CMAKE_BLUENET_SOURCE_DIR}/src/cfg/cs_AutoConfig.cpp.in" "${CMAKE_BLUENET_BINARY_DIR}/src/cfg/cs_AutoConfig.cpp")
list(APPEND GENERATED_SOURCES "${CMAKE_BLUENET_BINARY_DIR}/src/cfg/cs_AutoConfig.cpp")

LOGi("Configure cs_StaticConfig.h file")
configure_file("${CMAKE_BLUENET_SOURCE_DIR}/include/cfg/cs_StaticConfig.h.in" "${CMAKE_BLUENET_BINARY_DIR}/include/cfg/cs_StaticConfig.h")

LOGi("Configure cs_config.h file")
configure_file("${CMAKE_BLUENET_SOURCE_DIR}/include/third/nrf/cs_config.h.in" "${CMAKE_BLUENET_BINARY_DIR}/include/third/nrf/cs_config.h")

LOGi("Configure cs_MemoryLayout.h file")
configure_file("${CMAKE_BLUENET_SOURCE_DIR}/shared/cs_MemoryLayout.h.in" "${CMAKE_BLUENET_BINARY_DIR}/include/cs_MemoryLayout.h")

LOGi("current bindir: ${CMAKE_BLUENET_BINARY_DIR}")
include_directories(${CMAKE_BLUENET_BINARY_DIR}/include/third/nrf)
#link_directories(${CMAKE_CURRENT_BINARY_DIR}/include/third/nrf/sdk${NORDIC_SDK_VERSION_FULL})

# these includes don't have the
include_directories("include" "${CMAKE_BLUENET_BINARY_DIR}/include")

##################################
# Get all includes from crownstone
##################################

# add include directories relative to the CMAKE_BLUENET_SOURCE_DIR here:
set(HOST_INCLUDE_DIRS "include")

list(APPEND HOST_INCLUDE_DIRS "include/ble")
list(APPEND HOST_INCLUDE_DIRS "include/third")
list(APPEND HOST_INCLUDE_DIRS "include/third/nrf")
list(APPEND HOST_INCLUDE_DIRS "include/third/nrf/sdk${NORDIC_SDK_VERSION_FULL}")

# loop over all host includes, adding them and adding a mock include with higher priority.
foreach(cs_include_dir ${HOST_INCLUDE_DIRS})
	# add the normal include so that everything that isn't mocked doesn't need explicit mocking.
	LOGd("Add include directory: " ${CMAKE_BLUENET_SOURCE_DIR}/${cs_include_dir})
	include_directories("${CMAKE_BLUENET_SOURCE_DIR}/${cs_include_dir}")
endforeach()

foreach(cs_include_dir ${HOST_INCLUDE_DIRS})
	# Add mock include with higher prio.
	# Override header files by adding a mock file in the exact same relative location in the mock folder.
	LOGd("Add mock include directory: " ${CMAKE_BLUENET_MOCK_DIR}/${cs_include_dir})
	include_directories(BEFORE "${CMAKE_BLUENET_MOCK_DIR}/${cs_include_dir}")
endforeach()


##############################
# Get all includes from nordic
##############################

set (INCLUDE_BASE_DIR ${NRF5_HOST_DIR})
LOGd("INCLUDE_BASE_DIR: ${INCLUDE_BASE_DIR}")
add_compile_definitions("SVCALL_AS_NORMAL_FUNCTION")

# headers
include(nrf5-sdk-includes.src)


###################################################
# Include nrf5 sdk source files for platform host
###################################################

# sources
include(nrf5sdk-platform-independent.src)
include(nrf5sdk-platform-host.src)

# add platform dependent source files to `SOURCE_NRF5_HOST_ABS`
foreach(FILE_REL IN LISTS NRF5SDK_PLATFORM_HOST_SOURCE_REL)
	list(APPEND SOURCE_NRF5_HOST_ABS ${NRF5_HOST_DIR}/${FILE_REL})
endforeach()

# add platform independent sources to `SOURCE_NRF5_HOST_ABS`
foreach(FILE_REL IN LISTS NRF5SDK_PLATFORM_INDEPENDENT_SOURCE_REL)
	list(APPEND SOURCE_NRF5_HOST_ABS ${NRF5_HOST_DIR}/${FILE_REL})
endforeach()

if(NOT SOURCE_NRF5_HOST_ABS)
	LOGw("No sources provided for SOURCE_NRF5_HOST_ABS")
endif()

######################################
# Include crownstone test source files
######################################

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include(crownstone-testlib.src)

# add test source files to `TEST_SOURCE_ABS`
foreach(FILE_REL IN LISTS TEST_SOURCE_REL)
	list(APPEND TEST_SOURCE_ABS ${CMAKE_CURRENT_SOURCE_DIR}/src/${FILE_REL})
endforeach()

###################################################
# Include crownstone source files for platform host
###################################################

# Source directory
set(SOURCE_DIR "../source/src")

include(crownstone-platform-independent.src)
include(crownstone-platform-host.src)

if(NOT FOLDER_SOURCE)
	LOGw("No crownstone sources provided for ho(st build")
endif()

###################################################
# combine sources into a lib
###################################################

# add to folder sources
list(APPEND HOSTLIB_SOURCE
	${SOURCE_NRF5_HOST_ABS} 
	${FOLDER_SOURCE}
	${TEST_SOURCE_ABS}
	)

set(BLUENET_HOST_LIB BluenetHost)
LogListV("HOSTLIB_SOURCE: " HOSTLIB_SOURCE)
add_library(BluenetHost STATIC ${HOSTLIB_SOURCE})


##################################################################################
# This function defines a test executable, links it to the bluenet and
# nrf mock libs, and adds it to the ctest runner.
#
# @argument testpath: path to sourcefile of the test, relative to this cmake file.
##################################################################################

function(add_crownstone_test testpath)
	get_filename_component(TESTNAME ${testpath} NAME_WE)
	add_executable(${TESTNAME} "test/${testpath}")

	target_link_libraries(${TESTNAME} BluenetHost)
	
	add_test(NAME ${TESTNAME} COMMAND ${TESTNAME})
	install(TARGETS ${TESTNAME} RUNTIME DESTINATION ${CMAKE_BINARY_DIR})
endfunction()

###############################
# register test files for ctest
###############################

enable_testing()

include(crownstone-tests.src)

foreach(TEST_FILE IN LISTS TEST_SOURCE_FILES)
	LOGd("Adding testfile: " ${TEST_FILE})
	add_crownstone_test(${TEST_FILE})
endforeach()
