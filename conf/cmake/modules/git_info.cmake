function(git_info GIT_PATH GIT_BRANCH GIT_HASH)
  if(EXISTS ${GIT_PATH})
    # Note, these commands are Linux only. Check for other OS how to retrieve this info
    execute_process(COMMAND bash "-c" "cd ${GIT_PATH} && git symbolic-ref --short -q HEAD | tr -d '\n'" OUTPUT_VARIABLE BRANCH)
    execute_process(COMMAND bash "-c" "cd ${GIT_PATH} && git rev-parse --short=24 HEAD | tr -d '\n'" OUTPUT_VARIABLE HASH)
    set(${GIT_BRANCH} "${BRANCH}" PARENT_SCOPE)
    set(${GIT_HASH} "${HASH}" PARENT_SCOPE)
  else()
    message(FATAL_ERROR "Cannot find configuration file: ${GIT_PATH}")
  endif()
endfunction()

