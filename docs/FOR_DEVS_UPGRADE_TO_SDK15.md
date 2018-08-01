# Log of upgrading to SDK15

To download SDK15, navigate to the [Nordic website](https://www.nordicsemi.com/eng/Products/Bluetooth-low-energy/nRF5-SDK) or the [SDK download page](https://www.nordicsemi.com/eng/nordic/Products/nRF5-SDK/nRF5-SDK-zip/59011).
Download in particular version 15.0.0: <https://www.nordicsemi.com/eng/nordic/download_resource/59011/71/4720878/116085>

## Strategy

The largest refactoring is with respect to persistent storage. Also the way settings are stored requires cleaning up.
There is a lot of back and forth casting. A few convenient compile time template functions or macros should be 
sufficient. 

* First make it compile with the new SDK. Comment out most of the code.
* Make it run with the new SDK without segfaults.
* Make sure the functions are all working, but not yet with persistent memory (all within RAM).
* Write and use persistent memory functions.

## Observations

List of things:

* The source file for the comparator driver has been moved from `components/drivers_nrf/comp/nrf_drv_comp.c` to `modules/nrfx/drivers/src/nrfx_comp.c`. Update CMakeLists.txt `components/drivers_nrf/*/` to `modules/nrfx/drivers/src/` or `'modules/nrfx/hal/` and rename the files there.
* The file `nrf_delay.c` does not exist anymore. There is `components/libraries/delay/nrf_delay.h`.
* The file `app_timer_appsh.c` is gone. Needs now to be set in `sdk_config.h`.
* Upgrade to SoftDevice S132. This one is for the nRF52832. The S140 is for the nRF52840. It might be that the S132 will support Bluetooth 5 later as well [forum](https://devzone.nordicsemi.com/f/nordic-q-a/33450/s132-vs-s140).
* Softdevice handler library is different ([migration](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk5.v14.0.0%2Flib_softdevice_handler.html)). It is renamed from `softdevice_handler` to `nrf_sdh`, etc.
* Include `/modules/nrfx/mdk/` for `nrf52.h`.
* Lower-case: `CMSIS/Include` -> `cmsis/include`.
* Use templated file `config/nrf52832/config/sdk_config.h` in `./include/third/nrf`.
* Use templated file `modules/nrfx/templates/nRF52832/nrfx_config.h` in `./include/third`.
* Remove STRINGIFY from csUtils. Is now implemented by Nordic.
* It makes sense to use the Flash Data Storage functions. These are experimental, but `fstorage` itself is also labeled experimental. You can read more in the [documentation](http://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk5.v15.0.0%2Flib_fstorage.html&cp=4_0_0_3_49). Matters to consider: The overload per record seems quite high. There is a `NRF_FSTORAGE_SD_MAX_WRITE_SIZE` option that increases the changes for the SoftDevice to do the write operation in-between radio events. Note that the current mesh implementation is bare-metal and might still need love. 

There is a lot of casting and casting again... Let's see if we can remove it.

* Settings::get has as arguments `(uint8_t type, void* dest, uint16_t &size)`
* StorageHelper::getUint8(...), ::getUint16, ::getString, ::getArray<type> etc. cast all to this void pointer.

There are also many long functions, that do a lot of things almost the same. 

* For example, `publishUpdate` is called many times in a large switch statement. Maybe call it then every time. Or set a boolean for the few times that it does not need to be called.

There are a lot of monster functions with `void *` and then a range of things have to be cast to that target.

# Migration Tips

## Timer

The `app_timer_appsh` module has been removed.

Replace calls to `APP_TIMER_APPSH_INIT` with calls to `app_timer_init` and enable the `APP_TIMER_CONFIG_USE_SCHEDULER` option in `sdk_config.h`.

Change: `APP_TIMER_INIT(PRESCALER, OP_QUEUE_SIZE, SCHEDULER_FUNC);` to: `err_code = app_timer_init();`

The PRESCALER parameter has been removed from the APP_TIMER_TICKS macro. Therefore, you must change the following code:
`ticks = APP_TIMER_TICKS(MS, PRESCALER);` to: `ticks = APP_TIMER_TICKS(MS);`

`app_timer_cnt_diff_compute` API change - The tick diff value is now returned by the function and not by the pointer. Therefore, you must change the following code: `err_code = app_timer_cnt_diff_compute(ticks_to, ticks_from, &ticks_diff);` to: `ticks_diff = app_timer_cnt_diff_compute(ticks_to, ticks_from);`.

## Peer Manager

The device manager is now the peer manager. This module has been introduced to be able to handle central and peripheral 
devices simultaneously. Not really a use case for Crownstone, but device manager is obsolete now. Internally does
the peer manager rely on fstorage rather than pstorage.

[Tutorial](https://devzone.nordicsemi.com/tutorials/b/software-development-kit/posts/migrating-to-peer-manager).

And then it is newer than that again. Version v14. 

[Tutorial again](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk5.v14.0.0%2Flib_softdevice_handler.html)

`ble_evt_handler` instead of `pm_on_ble_evt`

`BLE_GATTS_EVT_HVN_TX_COMPLETE` vs `BLE_EVT_TX_COMPLETE`

# Common Errors

The following stems from using `C` constructs within `C++`:

`sorry, unimplemented: non-trivial designated initializers not supported`

This is probably because the C guys did not actually use the right order or forgot to include a field:

For example `.ext_ref` was forgotten in 

    .reference          = (nrf_comp_ref_t)NRFX_COMP_CONFIG_REF,             \
    .ext_ref            = (nrf_comp_ext_ref_t)0,                            \
    .main_mode          = (nrf_comp_main_mode_t)NRFX_COMP_CONFIG_MAIN_MODE, \

This is not related to using `extern "C"`.

# Unclear Decisions

There is a `third` directory with for example `src/third/nrf/app_timer.c`. It makes sense to assume that there have
been changes to the original files, but which ones?

# Nice to do next!

There is now a separate `ble_adv_primary_phy` and `ble_adv_secondary_phy` advertising GAP physical address.
