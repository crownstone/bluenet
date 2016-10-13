#!/bin/sh

path="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source $path/_utils.sh

browser=google-chrome

cd ..

log "Open browser at index.html in docs/html folder"
$browser docs/html/index.html
