# Log of upgrading to SDK15

To download SDK15, navigate to the [Nordic website](https://www.nordicsemi.com/eng/Products/Bluetooth-low-energy/nRF5-SDK) or the [SDK download page](https://www.nordicsemi.com/eng/nordic/Products/nRF5-SDK/nRF5-SDK-zip/59011).

Download In particular version 15.0.0: <https://www.nordicsemi.com/eng/nordic/download_resource/59011/71/4720878/116085>

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
