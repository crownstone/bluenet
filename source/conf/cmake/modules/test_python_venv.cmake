set(IS_VENV $ENV{VIRTUAL_ENV})

if(NOT IS_VENV)
	
	message(FATAL_ERROR "\nNot running in a virtual environment "
		"(while configured with -DPYTHON_SETUP_VENV=ON).\n"
		"Did you call:\n"
		"  source ${PYTHON_CROWNSTONE_VENV_PATH}/bin/activate"
		)
endif()

