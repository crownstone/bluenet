# Patch

Generate patch files preferably including part of the path...

For example, if there is a patch in the mesh SDK, create a copy of the mesh_sdk_patched in the tools directory.

We are in the parent directory of tools, where we run:

	diff -ruN tools/mesh_sdk/ tools/mesh_sdk_patched/ > patch/00mesh.patch

We can now apply the patch from the parent directory as well

	patch -p0 < patch/00mesh.patch
