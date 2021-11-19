include(${DEFAULT_MODULES_PATH}/get_versions.cmake)
include(${DEFAULT_MODULES_PATH}/git_info.cmake)

get_version_info("${WORKSPACE_DIR}/source" FIRMWARE_VERSION FIRMWARE_DFU_VERSION FIRMWARE_RC_VERSION)

message(STATUS "Firmware version: ${FIRMWARE_VERSION}")
message(STATUS "Firmware version: ${FIRMWARE_RC_VERSION}")

#if(FIRMWARE_RC_VERSION AND NOT FIRMWARE_RC_VERSION STREQUAL "") 
#  set(CROWNSTONE_RELEASE_DIR "${WORKSPACE_DIR}/release/crownstone_${FIRMWARE_VERSION}-${FIRMWARE_RC_VERSION}")
#else()
  # Hacked in: FIRMWARE_VERSION also has the "-RCx" added to it.
  set(CROWNSTONE_RELEASE_DIR "${WORKSPACE_DIR}/release/crownstone_${FIRMWARE_VERSION}")
#endif()

# Drop out if the directory already exists
if(EXISTS "${CROWNSTONE_RELEASE_DIR}") 
  message(FATAL_ERROR "Directory ${CROWNSTONE_RELEASE_DIR} already exists! Do you need to update the VERSION file?")
endif()

message(STATUS "Create release directory: ${CROWNSTONE_RELEASE_DIR}")
file(MAKE_DIRECTORY "${CROWNSTONE_RELEASE_DIR}")

message(STATUS "Copy conf/cmake/CMakeBuild.config.default file")
CONFIGURE_FILE(${WORKSPACE_DIR}/source/conf/cmake/CMakeBuild.config.default ${CROWNSTONE_RELEASE_DIR}/CMakeBuild.config COPYONLY)

git_info("${WORKSPACE_DIR}/source" GIT_BRANCH GIT_HASH)

message(STATUS "We are on the \"${GIT_BRANCH}\" branch")

if(NOT "${GIT_BRANCH}" STREQUAL "5.6.x") 
  message(FATAL_ERROR "We should be on the 5.6.x branch for release!")
endif()

git_local_changes("${WORKSPACE_DIR}/source" GIT_LOCAL_CHANGES)
if("${GIT_LOCAL_CHANGES}" STREQUAL "true") 
  #  message(FATAL_ERROR "We should have no local changes... Commit to git and try again.")
endif()

message(STATUS "Update git")
execute_process(COMMAND git remote update WORKING_DIRECTORY ${WORKSPACE_DIR}/source RESULT_VARIABLE exit_code OUTPUT_STRIP_TRAILING_WHITESPACE)
#message(STATUS "Result: ${exit_code}")

message(STATUS "Check if there is another revision on the server")
git_remote_changes("${WORKSPACE_DIR}/source" GIT_REMOTE_CHANGES)

if(FIRMWARE_RC_VERSION STREQUAL "") 
  message(STATUS "There is no RC version. This is a stable version!")
endif()

