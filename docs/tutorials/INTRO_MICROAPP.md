# Introduction microapps

The support for microapps is very preliminary. Be warned!

* Enable, flash, and setup the bluenet firmware
* Download an example project for a microapp

## Enable, flash, and setup

Make sure you are on the right branch (and now its limitations). There is support for microapps in the `master` branch, but it is old. You will likely want to have the `microapp` branch.

First enable support for microapps in e.g. `/config/$target/CMakeBuild.overwrite.config`.

```
BUILD_MICROAPP_SUPPORT=1
```

Flash the compiled binary to the target board and run **setup**. 

```
make write_config
make setup
```

Alternatively, set up the hardware through the Crownstone app. 

## Compile an example project

Clone an example project:

```
git clone https://github.com/crownstone/crownstone-microapp
```

In this repository you will see the code itself. First, adjust the location to the cross-compiler tool chain if necessary in the `config.mk` file:

```
GCC_PATH=$(HOME)/workspace/bluenet/tools/gcc_arm_none_eabi/bin
```

You also need a path to the most recent bluenet (make sure you have checked out the right branch and pulled the recent code):

```
# The path to the bluenet repository
BLUENET_PATH=$(HOME)/workspace/bluenet
```

Then compile in the usual sense with:

```
make
```

This will generate a `build/example.hex` file.

In the `/scripts` directory you will find a `upload_microapp.py` file which uses Bluetooth LE to upload the application the Crownstone running this bluenet version.
The syntax is something like:

```
./upload_microapp.py --keyFile ~/.secret_path/secret_keys.json -a C2:81:75:46:D8:F1 -f ../build/example.bin upload
```

You can also just use the Makefile, however in that case you will have to configure the above information in a file. Write the key file and the mac address to `private.mk` in the root of the microapp repository.

```
KEYS_JSON=~/.secret_path/secret_keys.json
MAC_ADDRESS=C2:81:75:46:D8:F1
```

And upload through:

```
make flash
```

Or, alternatively, over the air:

```
make ota-upload
```

Check for details the <https://github.com/crownstone/crownstone-microapp> README as well!

Important, when you upload a microapp it is not automatically enabled! This is a command that is executed automatically by the `upload_microapp.py` script. If you do **not** want to be dependent on Bluetooth, there is an option you can set in your `CMakeBuild.overwrite.config` file:

```
AUTO_ENABLE_MICROAPP_ON_BOOT=1
```

This removes the need of enabling the microapp over the air. Of course, in production we will always demand an enabling step and thus the default is off.

# What does not work yet?

## Reading

Currently, reading i2c is in the works. The code expects the bluenet firmware to be the master. This simplifies reading a lot (there's no asynchronicity required).
There is already some code in place that would work for the async case though.


