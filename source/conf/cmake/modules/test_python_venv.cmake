set(IS_VENV $ENV{VIRTUAL_ENV})
set(IS_CONDA $ENV{CONDA_PREFIX})

if(NOT IS_VENV AND NOT IS_CONDA)
	message(FATAL_ERROR "\nNot running in a virtual environment "
		"(while configured with -DPYTHON_SETUP_VENV=ON).\n"
		"Make sure to call:\n"
		"  source ${PYTHON_CROWNSTONE_VENV_PATH}/bin/activate\n"
		"Or activate a virtual env of your own choice."
		)
endif()

