# Specs of the bluenet firmware

The bluenet firmware does have a lot of sophisticated features:

| Feature                           | Description                                                            |
| ---                               | ---                                                                    |
| Mesh broadcast with cyclic buffer | A cyclic buffer for application specific data in the BLE mesh          |
| Persistent storage                | Persistent storage of certain state variables, e.g. switch state       |
| Flash write optimization          | Write optimization w.r.t. number of times that can be written to flash |
| Buffer reuse                      | Buffer reuse for different purposes                                    |
| App-specific scan responses       | Connection-less messages embedded in service data (scan responses)     |
| Encryption                        | Symmetric keys to encrypt connections and mesh messages                |

## Mesh broadcast with cyclic buffer

The bluenet-compatible devices can talk with each other over separate channels. A device can add additional state information by pushing it to a cyclic buffer that will be automatically broadcasted over all connected devices.

## Persistent storage

If the bluenet-compatible device is a power switch, it stores the state of a switch after it toggles. This allows someone to turn off the device, or plug it out, and have this information available after a reset. 

## Flash write optimization

To ensure that an individual device has a technical lifetime of 10 years, it is important to not write to the same flash location each time due to wear. On the nRF52 it is possible to write 10 000 times to the same flash location. To increase the number of times you can write a value to flash, it can be stored to multiple locations together with a pointer/counter. Using such a cyclic buffer the lifetime of the flash memory is increased considerably.

## Buffer reuse

A buffer abstraction that allows for the reuse of RAM for multiple purposes to limit memory use.

## App-specific scan responses

It is possible to obtain information about energy usage (in an encrypted manner) by encapsulating it in scan responses.

## Encryption

Regardless of BLE security measures there is payload encryption/decryption using SHA-1.

## Other features

There are plenty of other software considerations / optimizations.

+ While being connected there are still non-connectable advertisements sent to help with the indoor localization.
+ Establishing a dimmer is not trivial. Rather than relying on unreliable zero-crossing events either due to jitter or drift, a timer peripheral is gradually adjusted by zero-crossing events. Everything uses the CPU as little as possible. Nordic provides for this the so-called Programmable Peripheral Interconnect (PPI) event system.
