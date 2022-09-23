
##########################################################
# util function to print a list variable with preamble
#
# @argument preamble: prefixed to each log line
# @argument listvarname: cmake variable name to be printed
##########################################################


function(print_list preamble listvarname)
	if(${listvarname})
		foreach(F IN LISTS ${listvarname})
			message(STATUS "${preamble}${F}")
		endforeach()
	else() 
		message(STATUS "${preamble} << empty >>")
	endif()
endfunction()


##########################################################
# util function to print a status message,
# outputs only if verbosity level is high enoug.
#
# @arguments: all arguments are passed on to message(...)
##########################################################

function(LOGv)
	if(VERBOSITY GREATER_EQUAL 4)
		string(ASCII 27 ESCAPE)
		message(STATUS "${ESCAPE}[90m[V]${ESCAPE}[m " ${ARGV})
	endif()
endfunction()

function(LOGd)
	if(VERBOSITY GREATER_EQUAL 3)
		string(ASCII 27 ESCAPE)
		message(STATUS "${ESCAPE}[39m[D]${ESCAPE}[m " ${ARGV})
	endif()
endfunction()

function(LOGi)
	if(VERBOSITY GREATER_EQUAL 2)
		string(ASCII 27 ESCAPE)
		message(STATUS "${ESCAPE}[34m[I]${ESCAPE}[m " ${ARGV})
	endif()
endfunction()

function(LOGw)
	if(VERBOSITY GREATER_EQUAL 1)
		string(ASCII 27 ESCAPE)
		message(AUTHOR_WARNING "${ESCAPE}[33m[W]${ESCAPE}[m " ${ARGV})
	endif()
endfunction()

function(LOGe)
	string(ASCII 27 ESCAPE)
	message(FATAL_ERROR "${ESCAPE}[31m[E]${ESCAPE}[m " ${ARGV})
endfunction()


##########################################################
# util function to print a status messages, for each 
# element in a list.
# outputs only if verbosity level is high enough.
#
# @arguments: all arguments are passed on to message(...)
##########################################################

function(LogListV preamble listvarname)
	if(VERBOSITY GREATER_EQUAL 4)
		string(ASCII 27 ESCAPE)
		print_list("${ESCAPE}[90m[V]${ESCAPE}[m ${preamble}" ${listvarname})
	endif()
endfunction()

function(LogListD preamble listvarname)
	if(VERBOSITY GREATER_EQUAL 3)
		string(ASCII 27 ESCAPE)
		print_list("${ESCAPE}[39m[D]${ESCAPE}[m ${preamble}" ${listvarname})
	endif()
endfunction()
