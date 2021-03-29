# Logging

There are two manners with which Crownstone firmware supports logging:

* plain text logging
* binary logging

The firmware will send logs via UART. The UART device to be used depends on the configuration settings that you set at build or configuration time.
Likewise, if the firmware prints logs in either plain text or binary is a configuration setting as well.

```
CS_UART_BINARY_PROTOCOL_ENABLED=1
```

Regarding UART, make sure you have the right operating system permissions. For example, add your user to the dialout group, or use sudo.

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

Some remarks:

* If you attach a Crownstone and you do not know which firmware is on it, you will have a hard time getting the debug information out. Hence, it is a good habit to do regular public commits. For each public commit it is possible to check out that particular commit and then run the given script to be able to get the debug logs.
* If you attach a Crownstone and you do actually know that it is one with a version on it that is committed, you have to find out a way to get this information out. **Check**: there is no UART command yet to read out the firmware version over UART (see [UART_PROTOCOL](UART_PROTOCOL.md) and [lib doc](https://github.com/crownstone/crownstone-lib-python-uart/blob/master/DOCUMENTATION.md)).
* If you want to run a development kit or dongle from for example a PI or other type of hardware to do extended logging, you will need to install git, checkout the code base on that device and point the script towards the source files. There is no separate extraction step yet (which would allow you to do this with something like a `logging.db` file which you would generate on a host computer and which will be generally available for our releases).
* If you attach a Crownstone, there's always some output in plain text (so you know UART is operational). However, to actually start sending output, a command has to be sent over UART. It will not start logging automatically. This is done on purpose (we do not want some continuous logging in release).

Note that binary logging is **required** if you want to run everything on the Crownstone. You can only run plain text logging if you disable parts of its functionality.
