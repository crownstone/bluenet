cmake_minimum_required(VERSION 3.10)

project(bluenet-host)

option(HOST_TARGET ON)
option(DISABLE_NRF_HEADERS ON)

set(DEFAULT_CXX_FLAGS        "-std=c++17 -Wno-write-strings -fno-exceptions")
set(DEFAULT_C_FLAGS          "-std=gnu99")
set(DEFAULT_C_AND_CXX_FLAGS  "-g3 -Wall")

set(CMAKE_CXX_FLAGS          "${DEFAULT_CXX_FLAGS}"             CACHE STRING "C++ flags" FORCE)
set(CMAKE_C_FLAGS            "${DEFAULT_C_FLAGS}"               CACHE STRING "C flags" FORCE)
set(CMAKE_C_AND_CXX_FLAGS    "${DEFAULT_C_AND_CXX_FLAGS}"       CACHE STRING "C and C++ flags")

set(CMAKE_BLUENET_SOURCE_DIR_MOCK ${CMAKE_CURRENT_SOURCE_DIR}/../mock/source)
set(CMAKE_BLUENET_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../source)

set(CMAKE_BLUENET_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/bluenet)

if(HOST_TARGET)
	add_compile_definitions("HOST_TARGET")
endif()

if(DISABLE_NRF_HEADERS)
	add_compile_definitions("DISABLE_NRF_HEADERS")
endif()

add_compile_definitions("NRF52832_XXAA")

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

# On debug print all these macros
#message(STATUS "Config list: ${CONFIG_LIST}")

message(STATUS "Configure cs_Git.cpp file")
configure_file("${CMAKE_BLUENET_SOURCE_DIR}/src/cfg/cs_Git.cpp.in" "${CMAKE_BLUENET_BINARY_DIR}/src/cfg/cs_Git.cpp" @ONLY)
list(APPEND GENERATED_SOURCES "${CMAKE_BLUENET_BINARY_DIR}/src/cfg/cs_Git.cpp")

message(STATUS "Configure cs_AutoConfig.cpp file")
configure_file("${CMAKE_BLUENET_SOURCE_DIR}/src/cfg/cs_AutoConfig.cpp.in" "${CMAKE_BLUENET_BINARY_DIR}/src/cfg/cs_AutoConfig.cpp" @ONLY)
list(APPEND GENERATED_SOURCES "${CMAKE_BLUENET_BINARY_DIR}/src/cfg/cs_AutoConfig.cpp")

message(STATUS "Configure cs_StaticConfig.h file")
configure_file("${CMAKE_BLUENET_SOURCE_DIR}/include/cfg/cs_StaticConfig.h.in" "${CMAKE_BLUENET_BINARY_DIR}/include/cfg/cs_StaticConfig.h" @ONLY)

message(STATUS "Configure cs_config.h file")
configure_file("${CMAKE_BLUENET_SOURCE_DIR}/include/third/nrf/cs_config.h.in" "${CMAKE_BLUENET_BINARY_DIR}/include/third/nrf/cs_config.h" @ONLY)


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
	message(STATUS "Include Crownstone directory: " ${CMAKE_BLUENET_SOURCE_DIR}/${cs_include_dir})
	include_directories("${CMAKE_BLUENET_SOURCE_DIR}/${cs_include_dir}")
	
	# Add mock include with higher prio.
	# Override header files by adding a mock file in the exact same relative location in the mock folder.
	message(STATUS "Include Crownstone directory: " ${CMAKE_BLUENET_SOURCE_DIR_MOCK}/${cs_include_dir})
	include_directories(BEFORE "${CMAKE_BLUENET_SOURCE_DIR_MOCK}/${cs_include_dir}")
endforeach()


##############################
# Get all includes from nordic
##############################

include(nordic.src)


add_compile_definitions("SVCALL_AS_NORMAL_FUNCTION")


##############################################
# Include crownstone platform and mock sources
##############################################

# Source directory
set(SOURCE_DIR "../source/src")

include(crownstone-application.src)
include(crownstone-platform-mock.src)

###############################
# register test files for ctest
###############################

enable_testing()

# util function to reduce verbosity
function(add_crownstone_test testname)
add_executable(${testname} ${FOLDER_SOURCE} "test/${testname}.cpp")
add_test(NAME ${testname} COMMAND ${testname})
install(TARGETS ${testname} RUNTIME DESTINATION ${CMAKE_BINARY_DIR})
endfunction()

 
add_crownstone_test(test_InterleavedBuffer)
add_crownstone_test(test_HashFletcher32)
add_crownstone_test(test_BitmaskVarSize)
add_crownstone_test(test_SystemTimeSync)
add_crownstone_test(test_EventDispatcher)
