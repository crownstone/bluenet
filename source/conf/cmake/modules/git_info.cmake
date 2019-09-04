function(git_info GIT_PATH GIT_BRANCH GIT_HASH)
  if(EXISTS ${GIT_PATH})
    # Note, these commands are Linux only. Check for other OS how to retrieve this info
    execute_process(COMMAND git symbolic-ref --short -q HEAD WORKING_DIRECTORY ${GIT_PATH} OUTPUT_VARIABLE BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND git rev-parse --short=24 HEAD WORKING_DIRECTORY ${GIT_PATH} OUTPUT_VARIABLE HASH OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(${GIT_BRANCH} "${BRANCH}" PARENT_SCOPE)
    set(${GIT_HASH} "${HASH}" PARENT_SCOPE)
  else()
    message(FATAL_ERROR "Cannot find configuration file: ${GIT_PATH}")
  endif()
endfunction()

