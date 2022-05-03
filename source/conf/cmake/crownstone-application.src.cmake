######################################################################################################
# If a source file does not strictly depend on the platform to build for, it should be defined here.
######################################################################################################

message(STATUS "crownstone application source files appended to FOLDER_SOURCE")


LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/common/cs_Types.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/events/cs_Event.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/events/cs_EventDispatcher.cpp")
LIST(APPEND FOLDER_SOURCE "${SOURCE_DIR}/events/cs_EventListener.cpp")

list(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_BitmaskVarSize.cpp")
list(APPEND FOLDER_SOURCE "${SOURCE_DIR}/util/cs_Hash.cpp")


