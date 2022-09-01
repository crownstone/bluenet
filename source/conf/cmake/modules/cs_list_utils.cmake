
##########################################################
# util function to print a list variable with preamble
#
# @argument preamble: prefixed to each log line
# @argument listvarname: cmake variable name to be printed
##########################################################


function(print_list preamble listvarname)
	foreach(F IN LISTS ${listvarname})
		message(STATUS "${preamble} ${F}")
	endforeach()
endfunction()