######################################################################################################
# If a source file strictly depends on platform headers etc., it should be defined here.
######################################################################################################

message(STATUS "crownstone test sources appended to TEST_SOURCE_FILES")

LIST(APPEND TEST_SOURCE_FILES "test_InterleavedBuffer.cpp")
LIST(APPEND TEST_SOURCE_FILES "test_HashFletcher32.cpp")
LIST(APPEND TEST_SOURCE_FILES "test_BitmaskVarSize.cpp")
LIST(APPEND TEST_SOURCE_FILES "test_SystemTimeSync.cpp")
LIST(APPEND TEST_SOURCE_FILES "test_EventDispatcher.cpp")
LIST(APPEND TEST_SOURCE_FILES "test_BoardMap.cpp")
LIST(APPEND TEST_SOURCE_FILES "scenarios/test_ReleaseOverrideOnBehaviourUpdate.cpp")
LIST(APPEND TEST_SOURCE_FILES "scenarios/test_BehaviourConflictWithPresence.cpp")
LIST(APPEND TEST_SOURCE_FILES "storage/test_StorageWrite.cpp")
