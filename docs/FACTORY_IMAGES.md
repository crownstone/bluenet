# Factory images

The firmware release `.zip` files are exactly the same for all devices. This is on purpose so that our deployment in the field can be streamlined. We can do a DFU process independent of the target hardware.

However, during production, there are registers written so the firmware is able to establish which functions should be enabled. So for each type of hardware, we have to create an image.

The settings for each type of hardware are located in the dir `factory-images/hardware/`

These settings are then combined with software settings in the dir `factory-images/software/`

First, select the software and hardware settings:

```
cd build
cmake -DFACTORY_IMAGE=1 -DFACTORY_IMAGE_SOFTWARE_CONFIG=firmware-4.0.1_bootloader-2.0.0 -DFACTORY_IMAGE_HARDWARE_CONFIG=plug-zero-1B2G-patch-3 ..
```

Then, to create the factory image, we run:

```
make generate_factory_image
make install
```

This will generate the files in `bin/factory-images`.