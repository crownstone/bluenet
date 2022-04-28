# Host testing

The folder `/host/test/` contains device independent tests that can be ran from a host system.
These are compiled whenever compiling for a specific target configuration.  

with ctest:

```
cd build/host/$TARGET
ctest
```

or run the individual test executables in that same folder.