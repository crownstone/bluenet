# IDE

This document decribes the 

## Clion

To set the environment up in JetBrains Clion, add these options to cmake in preferences/settings -> build, execution, deployment -> Cmake

```
-DCMAKE_TOOLCHAIN_FILE=arm.toolchain.cmake
-DCOMPILATION_TIME='123'
-DGIT_BRANCH='test'
-DGIT_HASH='23123'
-DCMAKE_BUILD_TYPE=Debug
-DCMAKE_CXX_COMPILER_FORCED=true
-DCMAKE_C_COMPILER_FORCED=true
```

Also add the BLUENET_DIR and BLUENET_CONFIG_DIR to the environmental variables there.

## VSCode

To set the environment up in Microsoft Visual Code, add these options to the `.vscode/settings.json`.

```json
    "cmake.configureSettings": {
        "BOARD_TARGET": "default",
        "CMAKE_TOOLCHAIN_FILE": "source/arm.toolchain.cmake",
        "CMAKE_BUILD_TYPE": "Debug",
        "CMAKE_CXX_COMPILER_FORCED": "true",
        "CMAKE_C_COMPILER_FORCED": "true",
    },
    "cmake.generator": "Unix Makefiles",
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
    "C_Cpp.default.includePath": [
        "${workspaceFolder}/**",
    ],
```
