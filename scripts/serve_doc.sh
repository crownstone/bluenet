#!/bin/sh

source _utils.sh

browser=google-chrome

cd ..

log "Open browser at index.html in docs/html folder"
$browser docs/html/index.html
