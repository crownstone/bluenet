#!/bin/bash

# Get the scripts path: the path where this file is located.
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

# Generate key
nrfutil keys generate nrfutil_private_key.pem

# Show key
nrfutil keys display --key pk --format code nrfutil_private_key.pem
cs_info "Use the above code as dfu_public_key.c"
