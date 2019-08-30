function(compilation_info COMPILATION_DAY)
  # Note, these commands are Linux only. Check for other OS how to retrieve this info
  execute_process(COMMAND bash "-c" "date --iso=date | tr -d '\n'" OUTPUT_VARIABLE DAY)
  set(${COMPILATION_DAY} "${DAY}" PARENT_SCOPE)
endfunction()

