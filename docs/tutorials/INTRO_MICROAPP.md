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

## Download an example

Clone an example project:

```
git clone https://github.com/mrquincle/crownstone-bluenet-module
```

In this repository you will see the code itself. First, adjust the location to the cross-compiler tool chain if necessary in the `config.mk` file:

```
GCC_PATH=$(HOME)/workspace/bluenet/tools/gcc_arm_none_eabi/bin
```

Then compile in the usual sense with:

```
make
```

This will generate a `build/example.hex` file. 

In the `/scripts` directory you will find a `microapp.py` file which uses Bluetooth LE to upload the application the Crownstone running this bluenet version.
The syntax is something like:

```
./microapp.py ~/.secret_path/secret_keys.json C2:81:75:46:D8:F1 ../build/example.bin upload
```

However, there's also just a possibility to upload over UART. First write the MAC address to `private.mk` in the root.

```
KEYS_JSON=~/.secret_path/secret_keys.json
MAC_ADDRESS=C2:81:75:46:D8:F1
```

And upload through:

```
make flash
```

You will have to enable the microapp as well, which can only be done over the air at the moment.

```
make ota-enable
```

Check for details the <https://github.com/mrquincle/crownstone-microapp> README as well!

# What does not work yet?

## Reading

Currently, reading i2c is in the works. The code expects the bluenet firmware to be the master. This simplifies reading a lot (there's no asynchronicity required).
There is already some code in place that would work for the async case though.

## Enabling / validation

Currently there is an enable command required which has as a payload the address of a particular custom main function in the Arduino code. However, this means that you can't just start the microapp at its start address (we shift the PC with this information). The same is true for the checksum with which we check if the application is correct. On the moment this information is out-band while we want it in-band.

* Prepend the binary with a jump instruction towards the custom main function.
* Append or prepend the binary with a field that contains the checksum of the binary (excluding the indicated fields).

This will make generating the `bin` file slightly more complex (and depending on `srec_cat` or similar binary manipulation tools). However, it makes the upload process much easier. Note that the checksum that the over-the-air process uses will be a different one (which includes the other checksum).

