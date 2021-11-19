# Patches

## Prevent patching

There are two ways in which we try to prevent to bring out patches:

* The Mesh SDK we have forked at <https://github.com/crownstone/nRF5-SDK-for-Mesh> which means we can apply our patches towards the actual files. We use git branches for the different Mesh SDK versions. We have also uploaded the nRF5 SDK to github at <https://github.com/crownstone/nRF5_SDK>.
* Adding patched files to the `include/third/nrf` and `src/third/nrf` directories.

This means that you will probably **not** need to be able to apply a patch! The following is kept for future reference, but consider patching the branches in the above repositories instead.

## How to create a patch file and how to test and apply it

Generate patch files including only a part of the path (not the complete absolute path).

If there is a patch required in the Nordic SDK, create a copy of the entire checked out SDK, e.g. version 17.1.0:

	cd tools/nrf5_sdk
	cp -R 17.1.0 17.1.0patch

Now, in the parent directory of tools (overall workspace of bluenet), run:

	diff -ruN tools/nrf5_sdk/17.1.0 tools/nrf5_sdk/17.1.0patch > patch/sdk17.1.0/00nrf5.patch

Test the patch from the parent directory as follows:

	patch -p0 < patch/sdk17.1.0/00nrf5.patch

And add a `PATCH_COMMAND` to the `CMakeLists.txt` file:

	PATCH_COMMAND cd ${WORKSPACE_DIR} && patch -p0 < ${NORDIC_SDK_PATCH_PATH}/00nrf5.patch


