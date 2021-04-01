# Logging

There are two manners with which Crownstone firmware supports logging:

* plain text logging
* binary logging

The firmware will send logs using a UART (a universal asynchronous receiver-transmitter). The UART on the computer's
side can be specified in the target configuration settings.

```
UART_DEVICE=/dev/ttyUSB0
```

This can be set at build or configuration time.

The UART chip is made available through a character device file like `/dev/ttyUSB0` by the driver on your operating 
system. Make sure you have set the right operating system permissions. For example, add your user to the dialout group, or use sudo.

Whether the firmware prints the logs in plain text or in binary mode is a configuration setting as well.

```
CS_UART_BINARY_PROTOCOL_ENABLED=1
```

## Plain text logs

If the you use plain text UART, then use can use a program like minicom (`sudo apt install minicom`):

    make uart_client

## Binary logs

To log in a binary method (with compressed output) is the default.

To read the logs you need to install [bluenet-lib-logs](#https://github.com/crownstone/bluenet-lib-logs/). You can run the client through:

    make uart_binary_client

Note, that this does not work out of the box for any Crownstone you attach to UART. The binary logs depend on the version of the code on the Crownstone. The above command calls `scripts/log-client.py`:


```
from bluenet_logs import BluenetLogs
bluenetLogs = BluenetLogs()
bluenetLogs.setSourceFilesDir(sourceFilesDir)
```

The `sourceFilesDir` is by default the `source` directory in bluenet.

## Details

There are quite some details involved with respect to the UART, logging, release firmware. Here we try to clear up a
few potentional misconceptions.

* If you attach a Crownstone and you do not know which firmware is on it, you will have a hard time getting the debug information out. Hence, it is a good habit to do regular public commits. For each public commit it is possible to check out that particular commit and then run the `log-client.py` script to be able to get the debug logs.
* If you attach a Crownstone and you do actually know that it is one with a version on it that is committed, you have to find out a way to get this information out. There is no UART command yet to read out the firmware version (see [UART_PROTOCOL](UART_PROTOCOL.md) and [lib doc](https://github.com/crownstone/crownstone-lib-python-uart/blob/master/DOCUMENTATION.md)). If the Crownstone is operational and you have the encryption keys you can get the firmware version over Bluetooth.
* If you want to run a development kit or dongle from for example a PI or other type of hardware to do extended logging, you will need to install git, checkout the code base on that device and point the `log-client.py` script towards the source files. There is no separate extraction step yet [1].
* If you attach a Crownstone for which logging is enabled, there's some output in plain text (so you know that the UART is operational). There is no UART command to enable logging. In none of our releases is logging enabled [2]. Note that apart from logging settings, the UART support might be compiled in, but not enabled [3].

For logging there are several levels of verbosity, for example:

```
SERIAL_VERBOSITY=SERIAL_INFO
```

In releases, the following mode is used:

```
SERIAL_VERBOSITY=SERIAL_BYTE_PROTOCOL_ONLY
```

This means no logging through for example `LOGi("hi")` or `LOGw("hi")` calls. All these calls are not even compiled into the firmware. There is only support for the commands.

[1]. A separate extraction step is a logical next step for us to implement. This allow us to generate something like a `logging.db` file on a host computer. Then subsequently this can be used in tandem with the compiled firmware. This file is then generally available for releases as well. For now, releases do have logging completely disabled.

[2]. On the moment there is no command to enable logging. In the future there might be, but for now logging is completely disabled in releases.

[3]. This concerns release firmware. The Crownstone USB dongle has both TX and RX enabled. The other Crownstones have only RX enabled in setup mode (it can only receive commands, not respond) and have both TX and RX disabled in normal mode.

Note that binary logging is **required** if you want to run everything on the Crownstone (or no logging at all of course). You can only run plain text logging if you disable parts of its functionality.
