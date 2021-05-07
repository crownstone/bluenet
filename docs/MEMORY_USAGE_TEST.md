# Memory usage test

To test how much memory the firmware currently uses, you should build the firmware with the default config and:

```
CS_SERIAL_ENABLED=SERIAL_ENABLE_RX_AND_TX
BUILD_MICROAPP_SUPPORT=1
```

With this build flag enabled, the firmware will:

- Set many State configs
- Fill up behaviour
- Queue many mesh messages
- Etc.

After erasing and flashing your device:

```
cd build/your_target
make memory_usage_test_client
```

This will run the test multiple times, as it's timing sensitive.
The log will be stored in `mem-usage-test.log`.
