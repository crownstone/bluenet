# Release process

Mostly copied from firmware release process. So before releasing a bootloader check if the firmware release process was updated.

## Bluenet

The release process starts with updating version information. The bootloader version has to be bumped. This is an integer that is checked by the bootloader to make sure that the
bootloader version is actually newer than the previous release.

Update the `VERSION` file in `source/bootloader/VERSION`:
```
2.1.0
9
RC0
```
The first line is the version string in the form `<major>.<minor>.<patch>`.
The second line is the integer version (used for the DFU process), and should be increased by 1 each time.
The third line is empty for a release, and `RC<int>` for a release candidate.

For further instructions, we set the version in a variable:

```
CS_BL_VERSION="2.1.0-RC0"
```

Then to create a release:

```
cd build
make create_git_release_bootloader
```

This will create a `release/` directory with the version information in a subdirectory, e.g. `release/bootloader_2.1.0-RC0`.
Double check if the CMakeBuild.config is correct. Eventually add a CMakeBuild.config.overwrite file for your own
passkey file.
Now using this information, we will build everything for this release.

```
cd build
cmake -DCONFIG_DIR=release -DBOARD_TARGET=bootloader_${CS_BL_VERSION} -DCMAKE_BUILD_TYPE=Release ..
make
```

Now we can start generating the DFU packages that will be released:

```
cd bootloader_${CS_BL_VERSION}
make generate_dfu_package_all
make generate_dfu_package_bootloader
make install
```

After make install you will find the `.zip` files in (subdirectories of) `bin/bootloader_2.1.0-RC0`.

Don't forget to commit the release config:

```
git add source/bootloader/VERSION
git add release/bootloader_${CS_BL_VERSION}/CMakeBuild.config
git commit -m "Added bootloader ${CS_BL_VERSION}"
git push
```

And create a git tag:

```
git tag -a -m "Tagging bootloader version ${CS_BL_VERSION}" "bootloader-${CS_BL_VERSION}"
git push --tags
```

## Tests

Make sure you can upload the bootloader over the air.
Make sure you can still perform an OTA update. Both firmware, and a new bootloader should work.

## Release repository

After the above, the release `.zip` files are generated, however, they are not yet made available to the public.

There are two repositories that maintain releases.

* https://github.com/crownstone/bluenet-release
* https://github.com/crownstone/bluenet-release-candidate

The parent directory of the repositories is assumed to be the same as the parent directory of the bluenet repository.
This can be adjusted by the CMake variables `RELEASE_REPOSITORY` and `RELEASE_CANDIDATE_REPOSITORY` in the bluenet
project. Depending on the existence of RC (release candidate) version information in the `VERSION` file, the proper
repository will be used. To create a release directory and fill it, call:

```
cd build/bootloader_${CS_BL_VERSION}
make create_bootloader_release_in_repository
```

This will copy all `.zip`, `.elf`, `.bin`, files, as well as configuration files.


## Release to cloud

Now release the firmware to the cloud, so app users actually use the new bootloader. This is similar to the [application release](RELEASE_PROCESS.md#release-to-cloud).