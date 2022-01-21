# Logging

There are multiple manners with which Crownstone firmware supports logging:

1. UART binary logging. This is the default, and enables event data to be sent to the computer, and commands to be received from the computer.
2. UART plain text logging.
3. UART plain text logging with the NRF backend. Useful to get SDK logs, and crash reasons.
4. RTT logging, this requires a JLink and can be used next to UART logging 1 and 2. Useful to debug the UART, or to get mesh SDK logs.

When using options other than binary logging, you might run into problems with the size of the firmware binary. In that case, you will have to decrease the log verbosity:
- Bluenet: `SERIAL_VERBOSITY` in your configuration settings file `CMakeBuild.config`.
- SDK: variables like `NRF_LOG_DEFAULT_LEVEL` in `app_config.h`.
- Mesh: variables like in `LOG_LEVEL_DEFAULT` in `nrf_mesh_config_app.h`, and the settings passed to `__LOG_INIT()` in `cs_MeshCore.cpp`.

## Uart config

The firmware will send logs using a UART (a universal asynchronous receiver-transmitter). The UART on the computer's
side can be specified in the target configuration settings:

```
UART_DEVICE=/dev/ttyUSB0
```

To prevent a complete rebuild for just changing the UART device, define it in the [runtime config](INSTALL.md#overwrites-and-runtime-configs).


The UART chip is made available through a character device file like `/dev/ttyUSB0` by the driver on your operating system.
Make sure you have set the right operating system permissions. For example, add your user to the dialout group, or use sudo.

Whether the firmware prints the logs in plain text or in binary mode is a configuration setting as well:
```
CS_UART_BINARY_PROTOCOL_ENABLED=1
```

When using the NRF backend, you will have to manually configure the correct UART TX pin to use, at compile time:
```
CS_SERIAL_NRF_LOG_ENABLED=1
CS_SERIAL_NRF_LOG_PIN_TX=6
```


## Plain text logs

If the you use plain text UART, then use can use a program like minicom (`sudo apt install minicom`):

    make uart_client

## Binary logs

Binary logging strips log format strings from the firmware and puts them in a separate file. The binary log client uses this file to add the strings again on the client side. It's important to realise that the file with log strings has to be generated from the exact same source code that runs on your crownstone.

To generate the file with log strings, first compile the code, and then run `extract_logs`:

    cd build/default
    make
    make extract_logs

The generated file with log strings will end up in the `build/default` dir.

To read the logs you need to install [bluenet-lib-logs](#https://github.com/crownstone/bluenet-lib-logs/). You can run the client through:

    make uart_binary_client

The above command calls `scripts/log-client.py` with `UART_DEVICE` from your config as device and the generated file as log strings file.

## RTT logs

For RTT logs, you need to run a GDB server:

    cd build/default
    make debug_server

And the client in a separate terminal:

    make rtt_client

This will use the `RTT_PORT` from your config, or the default if none is defined. To prevent a complete rebuild for just changing the RTT port, define it in the [runtime config](INSTALL.md#overwrites-and-runtime-configs).


## Details

There are quite some details involved with respect to the UART, logging, release firmware. Here we try to clear up a
few potentional misconceptions.

* If you attach a Crownstone and you do not know which firmware is on it, you will have a hard time getting the debug information out. Hence, it is a good habit to do regular public commits. For each public commit it is possible to check out that particular commit and then run the `log-client.py` script to be able to get the debug logs.
* If you attach a Crownstone and you do actually know that it is one with a version on it that is committed, you have to find out a way to get this information out. There is no UART command yet to read out the firmware version (see [UART_PROTOCOL](UART_PROTOCOL.md) and [lib doc](https://github.com/crownstone/crownstone-lib-python-uart/blob/master/DOCUMENTATION.md)). If the Crownstone is operational and you have the encryption keys you can get the firmware version over Bluetooth.
* If you attach a Crownstone for which logging is enabled, there's some output in plain text (so you know that the UART is operational). There is no UART command to enable logging. In none of our releases is logging enabled [1]. Note that apart from logging settings, the UART support might be compiled in, but not enabled [2].

For logging there are several levels of verbosity, for example:

```
SERIAL_VERBOSITY=SERIAL_INFO
```

In releases, the following mode is used:

```
SERIAL_VERBOSITY=SERIAL_BYTE_PROTOCOL_ONLY
```

This means no logging through for example `LOGi("hi")` or `LOGw("hi")` calls. All these calls are not even compiled into the firmware. There is only support for the commands.

[1]. On the moment there is no command to enable logging. In the future there might be, but for now logging is completely disabled in releases.

[2]. This concerns release firmware. The Crownstone USB dongle has both TX and RX enabled. The other Crownstones have only RX enabled in setup mode (it can only receive commands, not respond) and have both TX and RX disabled in normal mode.

Note that binary logging is **required** if you want to run everything on the Crownstone (or no logging at all of course). You can only run plain text logging if you disable parts of its functionality.
