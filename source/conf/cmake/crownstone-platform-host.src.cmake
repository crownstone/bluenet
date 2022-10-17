###################################################################################################################
# If a source file cannot be compiled in a test because of platform dependence, a mock file should be defined here.
###################################################################################################################

message(STATUS "crownstone platform mock source files appended to FOLDER_SOURCE")

list(APPEND FOLDER_SOURCE "${CMAKE_BLUENET_SOURCE_DIR_MOCK}/util/cs_BleError.c")

LIST(APPEND FOLDER_SOURCE "${CMAKE_BLUENET_SOURCE_DIR_MOCK}/drivers/cs_PWM.cpp")
LIST(APPEND FOLDER_SOURCE "${CMAKE_BLUENET_SOURCE_DIR_MOCK}/drivers/cs_Relay.cpp")
LIST(APPEND FOLDER_SOURCE "${CMAKE_BLUENET_SOURCE_DIR_MOCK}/drivers/cs_RNG.cpp")
list(APPEND FOLDER_SOURCE "${CMAKE_BLUENET_SOURCE_DIR_MOCK}/drivers/cs_RTC.cpp")
LIST(APPEND FOLDER_SOURCE "${CMAKE_BLUENET_SOURCE_DIR_MOCK}/drivers/cs_Serial.cpp")
LIST(APPEND FOLDER_SOURCE "${CMAKE_BLUENET_SOURCE_DIR_MOCK}/drivers/cs_Storage.cpp")
list(APPEND FOLDER_SOURCE "${CMAKE_BLUENET_SOURCE_DIR_MOCK}/drivers/cs_Uicr.c")
#LIST(APPEND FOLDER_SOURCE "${CMAKE_BLUENET_SOURCE_DIR_MOCK}/storage/cs_State.cpp")
LIST(APPEND FOLDER_SOURCE "${CMAKE_BLUENET_SOURCE_DIR_MOCK}/drivers/cs_PWM.cpp")
