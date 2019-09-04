# Release Version

Things that have to be set in the CMakeBuild.config for a release version:

## Required

- define a firmware version to keep track of the version
FIRMWARE_VERSION="x.y.z"

- enable persistent storage for flags
PERSISTENT_FLAGS_DISABLED=0

- set default operation mode to setup so that the device starts up in setup mode
DEFAULT_OPERATION_MODE=OPERATION_MODE_SETUP

- define a default passkey, this passkey is used for encryption in setup mode
STATIC_PASSKEY="123456"

- enable the crownstone service with all it's characteristic. it is the one service that **HAS** to be present. all
others are optional
CROWNSTONE_SERVICE=1

## Optional but recommended:

- set the serial verbosity to NONE
SERIAL_VERBOSITY=SERIAL_NONE

