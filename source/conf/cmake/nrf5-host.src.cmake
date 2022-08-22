# We add the source files explicitly. This is recommended in the cmake system and will also force us all the time to
# consider the size of the final binary. Do not include things, if not necessary!

# The source files are added as paths to the  to the list variable NORDIC_SOURCE_NRF5_HOST.

list(APPEND SOURCE_NRF5_HOST_REL "components/libraries/timer/app_timer_host.cpp")
