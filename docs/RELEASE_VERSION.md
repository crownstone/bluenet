# Release process

The release process is as follows:

* Update the `VERSION` file in `source/VERSION` and `source/bootloader/VERSION`.
* Commit all the changes to github.

Then to release the particular `target` you have worked on, say `default`:

```
cd build
cmake -DCONFIG_DIR=config -DBOARD_TARGET=default -DCMAKE_BUILD_TYPE=Debug ..
make
cd default
make create_git_release
```

This will create a `release/` directory with the version information in a subdirectory, e.g. `release/crownstone_3.0.1-RC0`.
Now using this information, we will build everything for this release.

```
cd build
cmake -DCONFIG_DIR=release -DBOARD_TARGET=crownstone_3.0.1-RC0 -DCMAKE_BUILD_TYPE=Release ..
make 
```

Now we can start generating the DFU packages that will be released:

```
cd crownstone_3.0.1-RC0
make generate_dfu_package_application
make generate_dfu_package_bootloader
make install
```

After make install you will find the `.zip` files in (subdirectories of) `bin/crownstone_3.0.1-RC0`.


# CMakeBuild.config

The following definitions have to be set in the CMakeBuild.config for a release.

## Required

* Define a firmware version to keep track of the version: `FIRMWARE_VERSION="x.y.z"`.
* Enable persistent storage for flags: `PERSISTENT_FLAGS_DISABLED=0`.
* Set default operation mode to setup so that the device starts up in setup mode: `DEFAULT_OPERATION_MODE=OPERATION_MODE_SETUP`.
* Define a default passkey, this passkey is used for encryption in setup mode: `STATIC_PASSKEY="123456"`.
* Enable the crownstone service with all its characteristics. it is the one service that **HAS** to be present. All
other characteristics are optional: `CROWNSTONE_SERVICE=1`.

## Optional 

* Set the serial verbosity to NONE: `SERIAL_VERBOSITY=SERIAL_NONE`.

