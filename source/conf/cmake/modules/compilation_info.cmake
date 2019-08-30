function(compilation_info COMPILATION_DAY)
  # Note, these commands are Linux only. Check for other OS how to retrieve this info
  execute_process(COMMAND date "+%Y-%m-%d" HEAD OUTPUT_VARIABLE DAY OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(${COMPILATION_DAY} "${DAY}" PARENT_SCOPE)
endfunction()

