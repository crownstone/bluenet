message(STATUS "crownstone application source files appended to FOLDER_SOURCE")
message(STATUS ${SOURCE_DIR})


list(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_BitmaskVarSize.cpp")
list(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_Hash.cpp")
