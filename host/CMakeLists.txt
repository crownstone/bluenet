cmake_minimum_required(VERSION 3.10)

project(bluenet-host)

message(STATUS "Building bluenet-host project. NORDIC_HOST_SDK_TARGET:=" ${NORDIC_HOST_SDK_TARGET})


option(HOST_TARGET ON)
option(DISABLE_NRF_HEADERS ON)

set(DEFAULT_CXX_FLAGS        "-std=c++17 -Wno-write-strings -fno-exceptions")
set(DEFAULT_C_FLAGS          "-std=gnu99")
set(DEFAULT_C_AND_CXX_FLAGS  "-g3 -Wall")

set(CMAKE_CXX_FLAGS          "${DEFAULT_CXX_FLAGS}"             CACHE STRING "C++ flags" FORCE)
set(CMAKE_C_FLAGS            "${DEFAULT_C_FLAGS}"               CACHE STRING "C flags" FORCE)
set(CMAKE_C_AND_CXX_FLAGS    "${DEFAULT_C_AND_CXX_FLAGS}"       CACHE STRING "C and C++ flags")

set(CMAKE_BLUENET_MOCK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../mock/source)
set(CMAKE_BLUENET_SOURCE_DIR_MOCK ${CMAKE_BLUENET_MOCK_DIR}/src)
set(CMAKE_BLUENET_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../source)

set(CMAKE_BLUENET_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/bluenet)

message (STATUS "HOST_NRF5_DIR: ${HOST_NRF5_DIR}")
message(STATUS "arend: ${WORKSPACE_DIR}/tools/nrf5_host_sdk/${NORDIC_SDK_VERSION_FULL}")

if(HOST_TARGET)
	add_compile_definitions("HOST_TARGET")
endif()

if(DISABLE_NRF_HEADERS)
	add_compile_definitions("DISABLE_NRF_HEADERS")
endif()

add_compile_definitions("NRF52832_XXAA")

# add paths to our custom cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_BLUENET_SOURCE_DIR}/conf")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_BLUENET_SOURCE_DIR}/conf/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_BLUENET_SOURCE_DIR}/conf/cmake/modules")

message(STATUS "${CMAKE_MODULE_PATH}")

set(DEFAULT_CONFIG_FILE "${CMAKE_BLUENET_SOURCE_DIR}/conf/cmake/CMakeBuild.config.default")

include(load_configuration)
load_configuration(${DEFAULT_CONFIG_FILE} CONFIG_LIST)

# Only contains Nordic specific compiler options
#include(crownstone.defs)

# These should come from a CMakeBuild.config.default file
add_compile_definitions("SERIAL_VERBOSITY=${SERIAL_VERBOSITY}")

message(STATUS "Configure cs_Git.cpp file")
configure_file("${CMAKE_BLUENET_SOURCE_DIR}/src/cfg/cs_Git.cpp.in" "${CMAKE_BLUENET_BINARY_DIR}/src/cfg/cs_Git.cpp")
list(APPEND GENERATED_SOURCES "${CMAKE_BLUENET_BINARY_DIR}/src/cfg/cs_Git.cpp")

message(STATUS "Configure cs_AutoConfig.cpp file")
configure_file("${CMAKE_BLUENET_SOURCE_DIR}/src/cfg/cs_AutoConfig.cpp.in" "${CMAKE_BLUENET_BINARY_DIR}/src/cfg/cs_AutoConfig.cpp")
list(APPEND GENERATED_SOURCES "${CMAKE_BLUENET_BINARY_DIR}/src/cfg/cs_AutoConfig.cpp")

message(STATUS "Configure cs_StaticConfig.h file")
configure_file("${CMAKE_BLUENET_SOURCE_DIR}/include/cfg/cs_StaticConfig.h.in" "${CMAKE_BLUENET_BINARY_DIR}/include/cfg/cs_StaticConfig.h")

message(STATUS "Configure cs_config.h file")
configure_file("${CMAKE_BLUENET_SOURCE_DIR}/include/third/nrf/cs_config.h.in" "${CMAKE_BLUENET_BINARY_DIR}/include/third/nrf/cs_config.h")

######################################################
# util function to print a list variable with preamble
# args: full path to test, relative to this cmake file
######################################################


function(print_list preamble listvarname)
	foreach(F IN LISTS ${listvarname})
		message(STATUS "${preamble} ${F}")
	endforeach()
endfunction()


##################################
# Get all includes from crownstone
##################################

# these includes don't have the
include_directories("include" "${CMAKE_BLUENET_BINARY_DIR}/include")

# add include directories relative to the CMAKE_BLUENET_SOURCE_DIR here:
set(HOST_INCLUDE_DIRS "include")

LIST(APPEND HOST_INCLUDE_DIRS "include/third")
LIST(APPEND HOST_INCLUDE_DIRS "include/third/nrf")
LIST(APPEND HOST_INCLUDE_DIRS "include/third/nrf/sdk${NORDIC_SDK_VERSION_FULL}")

# loop over all host includes, adding them and adding a mock include with higher priority.
foreach(cs_include_dir ${HOST_INCLUDE_DIRS})
	# add the normal include so that everything that isn't mocked doesn't need explicit mocking.
	message(STATUS "Add include directory: " ${CMAKE_BLUENET_SOURCE_DIR}/${cs_include_dir})
	include_directories("${CMAKE_BLUENET_SOURCE_DIR}/${cs_include_dir}")
	
	# Add mock include with higher prio.
	# Override header files by adding a mock file in the exact same relative location in the mock folder.
	message(STATUS "Add mock include directory: " ${CMAKE_BLUENET_MOCK_DIR}/${cs_include_dir})
	include_directories(BEFORE "${CMAKE_BLUENET_MOCK_DIR}/${cs_include_dir}")
endforeach()


##############################
# Get all includes from nordic
##############################

include(nordic-includes.src)
include(nrf5-host.src)

add_compile_definitions("SVCALL_AS_NORMAL_FUNCTION")

if(SOURCE_NRF5_HOST_REL)
	#loop through NORDIC_SOURCE_REL and convert to absolute paths in SOURCE_NRF5_HOST_ABS
	foreach(FILE_NRF5_HOST_REL ${SOURCE_NRF5_HOST_REL})
		list(APPEND SOURCE_NRF5_HOST_ABS ${HOST_NRF5_DIR}/${FILE_NRF5_HOST_REL})
	endforeach()

	set(NRF5_HOST_LIB NordicSdkHost)
	print_list("${NRF5_HOST_LIB} contains: " SOURCE_NRF5_HOST_ABS)
	add_library(${NRF5_HOST_LIB} STATIC ${SOURCE_NRF5_HOST_ABS})
else()
	message(WARNING "No sources provided for NRF5_HOST_LIB")
endif()

#################################################
# Include crownstone application and mock sources
#################################################

# Source directory
set(SOURCE_DIR "../source/src")

include(crownstone-application.src)
include(crownstone-platform-mock.src)

if(FOLDER_SOURCE)
	set(BLUENET_HOST_LIB BluenetHost)
	print_list("${BLUENET_HOST_LIB} contains: " FOLDER_SOURCE)
	add_library(${BLUENET_HOST_LIB} STATIC ${FOLDER_SOURCE})
else()
	message(WARNING "No sources provided for BLUENET_HOST_LIB")
endif()

#######################################################################
# util function to reduce verbosity
# args: full path to test, relative to this cmake file 
####################################################################### 

function(add_crownstone_test testpath)
	get_filename_component(TESTNAME ${testpath} NAME_WE)
	add_executable(${TESTNAME} "test/${testpath}")

	if(BLUENET_HOST_LIB)
		target_link_libraries(${TESTNAME} ${BLUENET_HOST_LIB})
	endif()

	if(NRF5_HOST_LIB)
		target_link_libraries(${TESTNAME} {NRF5_HOST_LIB})
	endif()
	
	add_test(NAME ${TESTNAME} COMMAND ${TESTNAME})
	install(TARGETS ${TESTNAME} RUNTIME DESTINATION ${CMAKE_BINARY_DIR})
endfunction()

###############################
# register test files for ctest
###############################

enable_testing()

# actual test definitions
add_crownstone_test(test_InterleavedBuffer.cpp)
add_crownstone_test(test_HashFletcher32.cpp)
add_crownstone_test(test_BitmaskVarSize.cpp)
add_crownstone_test(test_SystemTimeSync.cpp)
add_crownstone_test(test_EventDispatcher.cpp)
add_crownstone_test(test_BoardMap.cpp)
add_crownstone_test(scenarios/test_ReleaseOverrideOnBehaviourUpdate.cpp)
