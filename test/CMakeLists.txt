include_directories ( "../include" )

set(TEST test_MeshMessageState)
set(TEST_SOURCE_FILES ../src/protocol/mesh/cs_MeshMessageState.cpp)
set(SOURCE_FILES ${TEST}.cpp ${TEST_SOURCE_FILES}) 
add_executable(${TEST} ${SOURCE_FILES})
add_test(NAME ${TEST} COMMAND ${TEST})
