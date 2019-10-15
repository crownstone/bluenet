function(git_info GIT_PATH GIT_BRANCH GIT_HASH)
  if(EXISTS ${GIT_PATH})
    # Note, these commands are Linux only. Check for other OS how to retrieve this info
    execute_process(COMMAND git symbolic-ref --short -q HEAD WORKING_DIRECTORY ${GIT_PATH} OUTPUT_VARIABLE BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND git rev-parse --short=24 HEAD WORKING_DIRECTORY ${GIT_PATH} OUTPUT_VARIABLE HASH OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(${GIT_BRANCH} "${BRANCH}" PARENT_SCOPE)
    set(${GIT_HASH} "${HASH}" PARENT_SCOPE)
  else()
    message(FATAL_ERROR "The variable GIT_PATH=${GIT_PATH} is not set")
  endif()
endfunction()

# See https://stackoverflow.com/questions/35375186/get-git-status-boolean-without-slow-list-of-diff and
# https://stackoverflow.com/questions/2657935/checking-for-a-dirty-index-or-untracked-files-with-git
function(git_local_changes GIT_PATH GIT_LOCAL_CHANGES)
  if(EXISTS ${GIT_PATH})
    set(${GIT_LOCAL_CHANGES} "false" PARENT_SCOPE)
    execute_process(COMMAND git diff-files --quiet WORKING_DIRECTORY ${GIT_PATH} RESULT_VARIABLE exit_code OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(exit_code EQUAL "1")
      set(${GIT_LOCAL_CHANGES} "true" PARENT_SCOPE)
      message(STATUS "Working tree has changes (git diff-files)")
    endif()
    execute_process(COMMAND git diff-index --quiet --cached HEAD -- WORKING_DIRECTORY ${GIT_PATH} RESULT_VARIABLE exit_code OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(exit_code EQUAL "1")
      set(${GIT_LOCAL_CHANGES} "true" PARENT_SCOPE)
      message(STATUS "Index has changes that are staged, but not yet committed (git diff-index)")
    endif()
    execute_process(COMMAND git ls-files --others --exclude-standard WORKING_DIRECTORY ${GIT_PATH} OUTPUT_VARIABLE result OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT result STREQUAL "")
      set(${GIT_LOCAL_CHANGES} "true" PARENT_SCOPE)
      message(STATUS "There are untracked local files (git ls-files)")
    endif()
  else()
    message(FATAL_ERROR "The variable GIT_PATH=${GIT_PATH} is not set")
  endif()
endfunction()

function(git_remote_changes GIT_PATH GIT_REMOTE_CHANGES) 
  if(EXISTS ${GIT_PATH})
    set(${GIT_REMOTE_CHANGES} "false" PARENT_SCOPE)

    # Get remote branch name, probably origin/master
    execute_process(COMMAND git rev-parse --abbrev-ref @{u} WORKING_DIRECTORY ${GIT_PATH} OUTPUT_VARIABLE remote_branch_name OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REGEX REPLACE "/" ";" variable_list "${remote_branch_name}")
    list(GET variable_list 0 remote)
    list(GET variable_list 1 branch)

    # Get remote revision
    execute_process(COMMAND git ls-remote "${remote}" "${branch}" WORKING_DIRECTORY ${GIT_PATH} OUTPUT_VARIABLE remote_result OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REGEX REPLACE "\t" ";" variable_list "${remote_result}")
    list(GET variable_list 0 revision) 
    #message(STATUS "Remote revision is ${revision}")

    execute_process(COMMAND git rev-parse HEAD WORKING_DIRECTORY ${GIT_PATH} OUTPUT_VARIABLE local_revision OUTPUT_STRIP_TRAILING_WHITESPACE)
    #message(STATUS "Local revision is ${local_revision}")
  
    if(NOT local_revision STREQUAL revision)
      set(${GIT_REMOTE_CHANGES} "true" PARENT_SCOPE)
      message(STATUS "There is a remote revision available (git ls-remote)")
    endif()
  else()
    message(FATAL_ERROR "The variable GIT_PATH=${GIT_PATH} is not set")
  endif()
endfunction()


