# Add a configuration option

This is slightly different approach from actually adding files to the build system (as described [here](/docs/tutorials/ADD_BUILD_SUPPORT.md)).

1. Add configuration option(s)
2a. Adjust `cs_AutoConfig.cpp.in`, or
2b. Adjust `s_StaticConfig.h.in`, or
2c. Adjust `cs_config.h.in`.

# Adjust autoconfig

This is the recommended location to add a configuration option. It will be automatically picked up by `cmake` and is the right spot to set anything that is known
at configuration time.

Just add the variable you'd like to introduce to the firmware in [cs_AutoConfig.h](/source/include/cfg/cs_AutoConfig.h). For example,

```
extern const bool g_CONFIG_SPHERE_ID_DEFAULT;
```

And fill it with the right info (implemented through `cmake_configure_file`) in [cs_AutoConfig.cpp.in](/source/src/cfg/cs_AutoConfig.cpp.in)

```
const bool g_CONFIG_SPHERE_ID_DEFAULT = @SPHERE_ID@;
```

# Adjust static config

This is the file [cs_StaticConfig.h.in](/source/include/cfg/cs_StaticConfig.h.in). It is only a header file and can be used for variables that need to be known at compile time in a **static** sense.

# Adjust general config

This is the file [cs_config.h.in](/source/bootloader/cs_config.h.in). It is included from `app_config.h` and can be used to set settings that will need to be picked up by Nordic files.

