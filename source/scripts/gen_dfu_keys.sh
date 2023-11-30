#!/bin/bash

# Get the scripts path: the path where this file is located.
path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

# Set "key_to_pass" to store the generated key to "pass" instead of to unencrypted file.
key_to_pass=true
if [ $key_to_pass ]; then
	cs_info "Storing private key in pass"
	key_file=$(mktemp -u /tmp/tempfile.XXXXXX.cs)
	checkError
	trap "rm -f $key_file" EXIT
else
	key_file="nrfutil_private_key.pem"
	cs_warn "Storing private key unencrypted."
fi

cs_info "Generate key"
nrfutil keys generate "$key_file"
checkError

if [ $key_to_pass ]; then
	cs_info "Overwrite 'dfu-pkg-sign-key' entry in pass with new key? [y/N]"
	read store_pass_response
	if [[ ! $store_pass_response == "y" ]]; then
		cs_warn "abort"
		exit 1
	fi
	cat "$key_file" | pass insert --multiline dfu-pkg-sign-key
	checkError
fi

cs_info "Use the code below as dfu_public_key.c"
nrfutil keys display --key pk --format code "$key_file"
cs_info "Use the code above as dfu_public_key.c"
