include_directories ( "include" )

set(TEST test_InterleavedBuffer)

set(TEST_SOURCE_DIR "test/host")

# set(TEST_INCLUDE_FILES ${INCLUDE_DIR}/structs/buffer/cs_InterleavedBuffer.h)
set(SOURCE_FILES ${TEST_SOURCE_DIR}/${TEST}.cpp ${TEST_SOURCE_FILES}) 
add_executable(${TEST} ${SOURCE_FILES})
add_test(NAME ${TEST} COMMAND ${TEST})
