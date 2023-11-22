#!/bin/bash

# Set the default directory
export DIR=default

# Change to the Build directory
cd .. && \
cd build && \

# Execute the commands
make -j6 \
  && make erase \
  && make write_softdevice \
  && make write_mbr_param_address \
  && cd $DIR \
  && make write_board_version \
  && cd bootloader \
  && make write_bootloader \
  && make write_bootloader_address \
  && cd .. \
  && make write_application \
  && make build_bootloader_settings \
  && make write_bootloader_settings \
  && cd .. \
  && make reset \
  && cd $DIR \
  && make extract_logs \
#   && make uart_combined_client