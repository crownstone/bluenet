# Specs of the bluenet firmware

The bluenet firmware does have a lot of sophisticated features:

| Feature                           | Description                                                            |
| ---                               | ---                                                                    |
| Dual memory bank                  | On a firmware update, have option to roll back to previous version     |
| Mesh broadcast with cyclic buffer | A cyclic buffer for application specific data in the BLE mesh          |
| Perisistent storage               | Persistent storage of certain state variables, e.g. switch state       |
| Flash write optimization          | Write optimization w.r.t. number of times that can be written to flash |
| Buffer reuse                      | Buffer reuse for different purposes                                    |
| App-specific scan responses       | Connection-less messages embedded in service data (scan responses)     |
| Encryption                        | Symmetric keys to encrypt connections and mesh messages                |

## Dual memory bank

An nRF52 chip does have 512 kB flash. The bootloader occupies around 16 kB and the softdevice around 112 kB. This means that there is 400 kB available for the application. The bluenet app uses around 130 kB of that 400 kB, hence it fits comfortably twice into memory. That amount of space can be used to make the upload process of a new application more robust. 

A dual memory bank uses the second half of memory to store the new firmware. This firmware is uploaded to the bluenet-compatible device over the air, from a smartphone or laptop. Only when the checks are correct, this firmware will replace the old firmware. This reduces the risk of running firmware that is faulty (due to connections that break halfway the process or individual packet loss).

## Mesh broadcast with cyclic buffer

The bluenet-compatible devices can talk with each other over separate channels. A device can add additional state information by pushing it to a cyclic buffer that will be automatically broadcasted over all connected devices.

## Persistent storage

If the bluenet-compatible device is a power switch, it stores the state of a switch after it toggles. This allows someone to turn off the device, or plug it out, and have this information available after a reset. 

## Flash write optimization

To ensure that an individual device has a technical lifetime of 10 years, it is important to not write to the same flash location each time due to wear. On the nRF52 it is possible to write 10 000 times to the same flash location. To increase the number of times you can write a value to flash, it can be stored to multiple locations together with a pointer/counter. Using such a cyclic buffer the lifetime of the flash memory is increased considerably.


