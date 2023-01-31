###################################################################################################################
# If a source file cannot be compiled in a test because of platform dependence, a mock file should be defined here.
###################################################################################################################

message(STATUS "crownstone testlib source files appended to TEST_SOURCE_REL")

list(APPEND TEST_SOURCE_REL "typedevents/cs_BehaviourEvents.cpp")
list(APPEND TEST_SOURCE_REL "utils/cs_iostream.cpp")

