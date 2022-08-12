###################################################################################################################
# If a source file cannot be compiled in a test because of platform dependence, a mock file should be defined here.
###################################################################################################################

message(STATUS "crownstone platform mock source files appended to FOLDER_SOURCE")

list(APPEND FOLDER_SOURCE "${CMAKE_BLUENET_SOURCE_DIR_MOCK}/src/drivers/cs_Uicr.c")
list(APPEND FOLDER_SOURCE "${CMAKE_BLUENET_SOURCE_DIR_MOCK}/src/util/cs_BleError.c")
