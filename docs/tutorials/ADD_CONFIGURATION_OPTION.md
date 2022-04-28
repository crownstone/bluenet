# Add a configuration option

This is slightly different approach from actually adding files to the build system (as described [here](ADD_BUILD_SUPPORT.md)).

1. Add configuration option(s)
2. Adjust `cs_AutoConfig.cpp.in`, or
3. Adjust `s_StaticConfig.h.in`, or
4. Adjust `cs_config.h.in`.

## Add option to file

In [CMakeBuild.config.default](../../source/conf/cmake/CMakeBuild.config.default) add an option, say `SPHERE_ID = 1`.

In your own local configuration file, e.g. `/config/default/CMakeBuild.overwrite.config` (which does not exist on the github repository), you can of course set something else, say `SPHERE_ID = 234`.

# Adjust autoconfig

This is the recommended location to add a configuration option. It will be automatically picked up by `cmake` and is the right spot to set anything that is known
at configuration time.

Just add the variable you'd like to introduce to the firmware in [cs_AutoConfig.h](../../source/include/cfg/cs_AutoConfig.h). For example,

```
extern const bool g_CONFIG_SPHERE_ID_DEFAULT;
```

And fill it with the right info (implemented through `cmake_configure_file`) in [cs_AutoConfig.cpp.in](../../source/src/cfg/cs_AutoConfig.cpp.in)

```
const bool g_CONFIG_SPHERE_ID_DEFAULT = @SPHERE_ID@;
```

# Adjust static config

This is the file [cs_StaticConfig.h.in](../../source/include/cfg/cs_StaticConfig.h.in). It is only a header file and can be used for variables that need to be known at compile time in a **static** sense.

An example:

```
const uint16_t g_CS_CHAR_READ_BUF_SIZE = @MASTER_BUFFER_SIZE@;
```

This is convenient if you want to use the variable for example like this:

```
static const size_t CS_RESULT_PACKET_DEFAULT_PAYLOAD_SIZE = (g_CS_CHAR_READ_BUF_SIZE - sizeof(result_packet_header_t));
```

# Adjust general config

This is the file [cs_config.h.in](../../source/include/third/nrf/cs_config.h.in) for the firmware and the file [cs_config.h.in](../../source/bootloader/cs_config.h.in) for the bootloader. It is included from `app_config.h` and can be used to set settings that will need to be picked up by Nordic files.

For example, the SoftDevice itself might need to know certain settings which have to be indicated through macros, such as:

```
#define MAX_NUM_VS_SERVICES                      @MAX_NUM_VS_SERVICES@

#define ATTR_TABLE_SIZE                          @ATTR_TABLE_SIZE@

#define MESH_PERSISTENT_STORAGE                  @MESH_PERSISTENT_STORAGE@
```

Of course, only use this if necessary. The disadvantage of such global macros is that it complicates debugging (there is no way to print or watch a value), that it is actually depending on the order with which files are included (which can lead to subtle errors), that it is less amenable to C++ type checks, and that it is per definition very difficult to make dynamic. 
