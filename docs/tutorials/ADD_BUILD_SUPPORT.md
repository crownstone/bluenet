# Build support

To add support for particular functionality, you'll need to touch a few files. We will give an example using the i2c (also called twi) driver.

1. Add configuration option(s). 
2. Add Nordic files to the build system
3. Add your own files to the build system.
4. Add compiler flags (if necessary)

If you actually have to include new source files, use `BUILD_X`. If you only set parameters without the need for additional source files, only add the configuration option, but skip the rest of this tutorial. You only need to follow the [tutorial to add a configuration option](ADD_CONFIGURATION_OPTION.md).

## Add configuration options

In [CMakeBuild.config.default](../../source/conf/cmake/CMakeBuild.config.default) add an option, say `BUILD_TWI = 0` (the default is off for a new function). 

In your own local configuration file, e.g. `/config/default/CMakeBuild.overwrite.config`, set `BUILD_TWI = 1`.


## Add Nordic files

Very likely you will need additional files to be compiled. Use the files with the `nrfx_` prefix (the other ones are old and will be phased out in the end). 
The location where you can specify where this is done is in [source/CMakeLists.txt](../../source/CMakeLists.txt) (note, it is **not** in the root `/CMakeList.txt` file). 

```
IF (BUILD_TWI)
	LIST(APPEND NORDIC_SOURCE "${NRF5_DIR}/modules/nrfx/drivers/src/nrfx_twi.c")
ELSE()
	MESSAGE(STATUS "Module for twi disabled")
ENDIF()
```

## Add your own new files

The code you have written and which interfaces with the Nordic code also has to be included. This is through [crownstone.src.cmake](../../source/conf/cmake/crownstone.src.cmake).

```
IF (BUILD_TWI)
	LIST(APPEND FOLDER_SOURCE "${BLUENET_SRC_DIR}/drivers/cs_Twi.cpp")
ENDIF()
```

## Add compiler flags

Optionally, in [crownstone.defs.cmake](../../source/conf/cmake/crownstone.defs.cmake) add a compiler flag so the cross-compiler (or even the precompiler) is aware of the new functionality.
Of course, only do this if necessary.

```
# Add twi driver
ADD_DEFINITIONS("-DBUILD_TWI=${BUILD_TWI}")
``` 

# What's next?

Now you can start coding. See e.g. [this tutorial](ADD_NEW_COMMAND.md) on how to add a **command** or **event types**. One of the first things you will see if you browse the code is
that there is an event bus, so we'll quickly review this here. 

## Event bus

You can derive from an EventListener to get all kind of events from other modules in the bluenet firmware. This is to have a system that is as decoupled as possible.

```
class Twi: public EventListener {
	...  
	void handleEvent(event_t & event);
};
```

If you have defined your own event, e.g. `CS_TYPE::EVT_TWI_INIT` you can capture it here.

```
void Twi::handleEvent(event_t & event) {
	switch(event.type) {
		case CS_TYPE::EVT_TWI_INIT: 
		...
	}
}
```

Sending an event is through something like:

```
TYPIFY(EVT_TWI_INIT) twiValue = val;
event_t event(CS_TYPE::EVT_TWI_INIT, &twiValue, sizeof(twiValue));
EventDispatcher::getInstance().dispatch(event);
```

There's even a mechanism to send an event on the event bus and gets a value back. However, it is strongly recommended **not** to use that. 
It's a better practice to have the other party sending its own event. 

Happy coding!
