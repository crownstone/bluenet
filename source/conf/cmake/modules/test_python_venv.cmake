set(IS_VENV $ENV{VIRTUAL_ENV})
set(IS_CONDA $ENV{CONDA_PREFIX})

if(NOT IS_VENV AND NOT IS_CONDA)
	message(FATAL_ERROR "\nNot running in a virtual environment "
		"(while configured with -DPYTHON_SETUP_VENV=ON).\n"
		"Did you call:\n"
		"  source ${PYTHON_CROWNSTONE_VENV_PATH}/bin/activate"
		)
endif()

