# NRF error codes

The errors are quite hard to track. There is not a single list. So, here, for your convenience:

    #define NRF_ERROR_BASE_NUM      (0x0)       ///< Global error base
    #define NRF_ERROR_SDM_BASE_NUM  (0x1000)    ///< SDM error base
    #define NRF_ERROR_SOC_BASE_NUM  (0x2000)    ///< SoC error base
    #define NRF_ERROR_STK_BASE_NUM  (0x3000)    ///< STK error base

Global errors:

    #define NRF_SUCCESS                           (NRF_ERROR_BASE_NUM + 0)  ///< Successful command
    #define NRF_ERROR_SVC_HANDLER_MISSING         (NRF_ERROR_BASE_NUM + 1)  ///< SVC handler is missing
    #define NRF_ERROR_SOFTDEVICE_NOT_ENABLED      (NRF_ERROR_BASE_NUM + 2)  ///< SoftDevice has not been enabled
    #define NRF_ERROR_INTERNAL                    (NRF_ERROR_BASE_NUM + 3)  ///< Internal Error
    #define NRF_ERROR_NO_MEM                      (NRF_ERROR_BASE_NUM + 4)  ///< No Memory for operation
    #define NRF_ERROR_NOT_FOUND                   (NRF_ERROR_BASE_NUM + 5)  ///< Not found
    #define NRF_ERROR_NOT_SUPPORTED               (NRF_ERROR_BASE_NUM + 6)  ///< Not supported
    #define NRF_ERROR_INVALID_PARAM               (NRF_ERROR_BASE_NUM + 7)  ///< Invalid Parameter
    #define NRF_ERROR_INVALID_STATE               (NRF_ERROR_BASE_NUM + 8)  ///< Invalid state, operation disallowed in this state
    #define NRF_ERROR_INVALID_LENGTH              (NRF_ERROR_BASE_NUM + 9)  ///< Invalid Length
    #define NRF_ERROR_INVALID_FLAGS               (NRF_ERROR_BASE_NUM + 10) ///< Invalid Flags
    #define NRF_ERROR_INVALID_DATA                (NRF_ERROR_BASE_NUM + 11) ///< Invalid Data
    #define NRF_ERROR_DATA_SIZE                   (NRF_ERROR_BASE_NUM + 12) ///< Data size exceeds limit
    #define NRF_ERROR_TIMEOUT                     (NRF_ERROR_BASE_NUM + 13) ///< Operation timed out
    #define NRF_ERROR_NULL                        (NRF_ERROR_BASE_NUM + 14) ///< Null Pointer
    #define NRF_ERROR_FORBIDDEN                   (NRF_ERROR_BASE_NUM + 15) ///< Forbidden Operation
    #define NRF_ERROR_INVALID_ADDR                (NRF_ERROR_BASE_NUM + 16) ///< Bad Memory Address
    #define NRF_ERROR_BUSY                        (NRF_ERROR_BASE_NUM + 17) ///< Busy
    #define NRF_ERROR_CONN_COUNT                  (NRF_ERROR_BASE_NUM + 18) ///< Maximum connection count exceeded.
    #define NRF_ERROR_RESOURCES                   (NRF_ERROR_BASE_NUM + 19) ///< Not enough resources for operation


STK errors of the form `0x3X..`:

    #define NRF_L2CAP_ERR_BASE             (NRF_ERROR_STK_BASE_NUM+0x100) /**< L2CAP specific errors. */
    #define NRF_GAP_ERR_BASE               (NRF_ERROR_STK_BASE_NUM+0x200) /**< GAP specific errors. */
    #define NRF_GATTC_ERR_BASE             (NRF_ERROR_STK_BASE_NUM+0x300) /**< GATT client specific errors. */
    #define NRF_GATTS_ERR_BASE             (NRF_ERROR_STK_BASE_NUM+0x400) /**< GATT server specific errors. */

STK errors of the form `0x3..X`:

    #define BLE_ERROR_NOT_ENABLED            (NRF_ERROR_STK_BASE_NUM+0x001) /**< @ref sd_ble_enable has not been called. */
    #define BLE_ERROR_INVALID_CONN_HANDLE    (NRF_ERROR_STK_BASE_NUM+0x002) /**< Invalid connection handle. */
    #define BLE_ERROR_INVALID_ATTR_HANDLE    (NRF_ERROR_STK_BASE_NUM+0x003) /**< Invalid attribute handle. */
    #define BLE_ERROR_NO_TX_BUFFERS          (NRF_ERROR_STK_BASE_NUM+0x004) /**< Buffer capacity exceeded. */
    #define BLE_ERROR_INVALID_ROLE           (NRF_ERROR_STK_BASE_NUM+0x005) /**< Invalid role. */



