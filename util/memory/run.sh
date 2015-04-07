#!/bin/bash

echo "Make sure you did run ../../scripts/map_update.sh once"

options="--new-window --allow-file-access-from-files"
google-chrome $options index.html
