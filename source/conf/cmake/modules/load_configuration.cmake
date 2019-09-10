function(load_configuration CONFIG_FILE CONFIG_LIST)
  if(EXISTS ${CONFIG_FILE})
    file(STRINGS ${CONFIG_FILE} ConfigContents)
    foreach(NameAndValue ${ConfigContents})
      # Strip leading spaces
      string(REGEX REPLACE "^[ ]+" "" NameAndValue ${NameAndValue})
      # Ignore comments
      string(REGEX REPLACE "^\#.*" "" NameAndValue ${NameAndValue})

      if(NOT NameAndValue STREQUAL "")
        # Find variable name
        string(REGEX MATCH "^[^=]+" Name ${NameAndValue})
        # Find the value
        string(REPLACE "${Name}=" "" Value ${NameAndValue})

        if(NOT Value STREQUAL "") 
          # Strip double quotes
          string(REGEX REPLACE "\"" "" Value ${Value})
          set(${Name} "${Value}")
          if(VERBOSITY GREATER 4)
            message(STATUS "CMakeConfig [${CONFIG_FILE}]: Set ${Name} to ${Value}")
          endif()
          # Note, we use STRING as type actually this should be BOOL, FILEPATH, etc.
          list(APPEND VAR_LIST "-D${Name}:STRING=${Value}")
          #if (Name STREQUAL "BLUETOOTH_NAME") 
          #  message(STATUS "Use BLE name ${Value}")
          #endif()
        endif()
      endif()
    endforeach()
  else()
    message(FATAL_ERROR "Cannot find configuration file: ${CONFIG_FILE}")
  endif()
  # Append original variable list to the result
  list(APPEND TOTAL_LIST ${${CONFIG_LIST}})
  list(APPEND TOTAL_LIST ${VAR_LIST})
  set(${CONFIG_LIST} "${TOTAL_LIST}" PARENT_SCOPE)
endfunction()

