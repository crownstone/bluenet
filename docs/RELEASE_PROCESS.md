# Release process

## Memory usage test

Perform a [memory usage test](MEMORY_USAGE_TEST.md), and commit the result.

## Bluenet

The release process starts with updating version information. The application version has to be bumped. This is an integer that is checked by the bootloader to make sure that the
application is actually newer than the previous release.

Update the `VERSION` file in `source/VERSION`:
```
3.0.1
54
RC0
```
The first line is the version string in the form `<major>.<minor>.<patch>`.
The second line is the integer version (used for the DFU process), and should be increased by 1 each time.
The third line is empty for a release, and `RC<int>` for a release candidate.

For further instructions, we set the version in a variable:

```
CS_FW_VERSION="3.0.1-RC0"
```

Then to create a release:

```
cd build
make create_git_release
```

This will create a `release/` directory with the version information in a subdirectory, e.g. `release/crownstone_3.0.1-RC0`.
Double check if the CMakeBuild.config is correct. Eventually add a CMakeBuild.config.overwrite file for your own
passkey file.
Now using this information, we will build everything for this release.

```
cd build
cmake -DCONFIG_DIR=release -DBOARD_TARGET=crownstone_${CS_FW_VERSION} -DCMAKE_BUILD_TYPE=Release -DFACTORY_IMAGE= ..
make -j
```

Now we can start generating the DFU packages that will be released:

```
cd crownstone_${CS_FW_VERSION}
make build_bootloader_settings
make generate_dfu_package_all
make generate_dfu_package_application
make generate_dfu_package_bootloader
make install
```

After make install you will find the `.zip` files in (subdirectories of) `bin/crownstone_3.0.1-RC0`.

Don't forget to commit the release config:

```
git add source/VERSION
git add release/crownstone_${CS_FW_VERSION}/CMakeBuild.config
git commit -m "Added ${CS_FW_VERSION}"
git push
```

And create a git tag:

```
git tag -a -m "Tagging version ${CS_FW_VERSION}" "v${CS_FW_VERSION}"
git push --tags
```

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
cd build/crownstone_${CS_FW_VERSION}
make create_release_in_repository
```

This will copy all `.zip`, `.elf`, `.bin`, files, as well as configuration files.
