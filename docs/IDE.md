# Clion

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
