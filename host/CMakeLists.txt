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

include_directories( "../source/include" "include" "${CMAKE_BLUENET_BINARY_DIR}/include")

# Get all includes from nordic
include(nordic.src)

set(THIRD_PARTY_INCLUDE_PATH "${CMAKE_BLUENET_SOURCE_DIR}/include/third")
set(THIRD_PARTY_NRF_INCLUDE_PATH "${CMAKE_BLUENET_SOURCE_DIR}/include/third/nrf")
set(THIRD_PARTY_NRF_SDK_INCLUDE_PATH "${CMAKE_BLUENET_SOURCE_DIR}/include/third/nrf/sdk${NORDIC_SDK_VERSION_FULL}")

message(STATUS "Include path: ${THIRD_PARTY_INCLUDE_PATH}")
message(STATUS "Include path: ${THIRD_PARTY_NRF_INCLUDE_PATH}")
message(STATUS "Include path: ${THIRD_PARTY_NRF_SDK_INCLUDE_PATH}")

include_directories(${THIRD_PARTY_INCLUDE_PATH})
include_directories(${THIRD_PARTY_NRF_INCLUDE_PATH})
include_directories(${THIRD_PARTY_NRF_SDK_INCLUDE_PATH})

add_compile_definitions("SVCALL_AS_NORMAL_FUNCTION")

set(TEST_SOURCE_DIR "test")


### --- register test files for ctest

enable_testing()

# util function to reduce verbosity
function(add_crownstone_test testname)
add_executable(${testname} ${TEST_SOURCE_FILES} "test/${testname}.cpp")
add_test(NAME ${testname} COMMAND ${testname})
install(TARGETS ${testname} RUNTIME DESTINATION ${CMAKE_BINARY_DIR})
endfunction()

# TODO(#180407661, Arend) @ANNE: I'd like to move this out into a separate file. 
# If we have to include dependencies explicitly it should be as simple as 'all of bluenet', except for a mock layer.
# That means we basically need 3 lists: 
#     platform independent bluenet sources, 
#     platform dependend bluenet sources, and 
#     mock for platform dependend sources.
# effectively splitting crownstone.src.cmake into 2 pieces and adding the mock layer as new part.

# Question(Arend) @ANNE: what is CMakeLists.host_target.txt doing in source/conf/cmake? Is that legacy?

list(APPEND TEST_SOURCE_FILES "../source/src/util/cs_BitmaskVarSize.cpp")
list(APPEND TEST_SOURCE_FILES "../source/src/util/cs_Hash.cpp")
list(APPEND TEST_SOURCE_FILES "../source/src/time/cs_SystemTime.cpp")
# E.G.: SystemTime needs Timer.cpp which needs to be mocked and Event.cpp, which is still host compatible
 
add_crownstone_test(test_InterleavedBuffer)
add_crownstone_test(test_HashFletcher32)
add_crownstone_test(test_BitmaskVarSize)
add_crownstone_test(test_SystemTimeSync)

