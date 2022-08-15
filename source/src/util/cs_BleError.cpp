/**
 * Author: Crownstone Team
 * Date: 21 Sep., 2013
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <logging/cs_Logger.h>
#include <util/cs_BleError.h>

//! Called by BluetoothLE.h classes when exceptions are disabled.
void ble_error_handler(const char* msg, uint32_t line_num, const char* p_file_name) {
	volatile const char* message __attribute__((unused)) = msg;
	volatile uint16_t line __attribute__((unused))       = line_num;
	volatile const char* file __attribute__((unused))    = p_file_name;

	LOGf("FATAL ERROR %s, at %s:%d", message, file, line);

	NRF_BREAKPOINT_COND;
	NVIC_SystemReset();
	//	__asm("BKPT");
	//	while (1) {}
}

/**
 * Check the following website for the error codes. They change with every SoftDevice release, so check them
 * regularly!
 *
 * https://infocenter.nordicsemi.com/topic/com.nordic.infocenter.s132.api.v6.0.0/group___b_l_e___c_o_m_m_o_n.html
 */
constexpr const char* nordicTypeName(uint32_t nordic_type) {
	switch (nordic_type) {
		case NRF_ERROR_SVC_HANDLER_MISSING:  // 0x0001
			return "NRF_ERROR_SVC_HANDLER_MISSING";
		case NRF_ERROR_SOFTDEVICE_NOT_ENABLED:  // 0x0002
			return "NRF_ERROR_SOFTDEVICE_NOT_ENABLED";
		case NRF_ERROR_INTERNAL:  // 0x0003
			return "NRF_ERROR_INTERNAL";
		case NRF_ERROR_NO_MEM:  // 0x0004
			return "NRF_ERROR_NO_MEM";
		case NRF_ERROR_NOT_FOUND:  // 0x0005
			return "NRF_ERROR_NOT_FOUND";
		case NRF_ERROR_NOT_SUPPORTED:  // 0x0006
			return "NRF_ERROR_NOT_SUPPORTED";
		case NRF_ERROR_INVALID_PARAM:  // 0x0007
			return "NRF_ERROR_INVALID_PARAM";
		case NRF_ERROR_INVALID_STATE:  // 0x0008
			return "NRF_ERROR_INVALID_STATE";
		case NRF_ERROR_INVALID_LENGTH:  // 0x0009
			return "NRF_ERROR_INVALID_LENGTH";
		case NRF_ERROR_INVALID_FLAGS:  // 0x0010
			return "NRF_ERROR_INVALID_FLAGS";
		case NRF_ERROR_INVALID_DATA:  // 0x0011
			return "NRF_ERROR_INVALID_DATA";
		case NRF_ERROR_DATA_SIZE:  // 0x0012
			return "NRF_ERROR_DATA_SIZE";
		case NRF_ERROR_TIMEOUT:  // 0x0013
			return "NRF_ERROR_TIMEOUT";
		case NRF_ERROR_NULL:  // 0x0014
			return "NRF_ERROR_NULL";
		case NRF_ERROR_FORBIDDEN:  // 0x0015
			return "NRF_ERROR_FORBIDDEN";
		case NRF_ERROR_INVALID_ADDR:  // 0x0016
			return "NRF_ERROR_INVALID_ADDR";
		case NRF_ERROR_BUSY:  // 0x0017
			return "NRF_ERROR_BUSY";
		case NRF_ERROR_CONN_COUNT: return "NRF_ERROR_CONN_COUNT";
		case NRF_ERROR_RESOURCES: return "NRF_ERROR_RESOURCES";
		case NRF_ERROR_SDM_LFCLK_SOURCE_UNKNOWN: return "NRF_ERROR_SDM_LFCLK_SOURCE_UNKNOWN";
		case NRF_ERROR_SDM_INCORRECT_INTERRUPT_CONFIGURATION: return "NRF_ERROR_SDM_INCORRECT_INTERRUPT_CONFIGURATION";
		case NRF_ERROR_SDM_INCORRECT_CLENR0: return "NRF_ERROR_SDM_INCORRECT_CLENR0";
		case NRF_ERROR_SOC_MUTEX_ALREADY_TAKEN: return "NRF_ERROR_SOC_MUTEX_ALREADY_TAKEN";
		case NRF_ERROR_SOC_NVIC_INTERRUPT_NOT_AVAILABLE: return "NRF_ERROR_SOC_NVIC_INTERRUPT_NOT_AVAILABLE";
		case NRF_ERROR_SOC_NVIC_INTERRUPT_PRIORITY_NOT_ALLOWED:
			return "NRF_ERROR_SOC_NVIC_INTERRUPT_PRIORITY_NOT_ALLOWED";
		case NRF_ERROR_SOC_NVIC_SHOULD_NOT_RETURN: return "NRF_ERROR_SOC_NVIC_SHOULD_NOT_RETURN";
		case NRF_ERROR_SOC_POWER_MODE_UNKNOWN: return "NRF_ERROR_SOC_POWER_MODE_UNKNOWN";
		case NRF_ERROR_SOC_POWER_POF_THRESHOLD_UNKNOWN: return "NRF_ERROR_SOC_POWER_POF_THRESHOLD_UNKNOWN";
		case NRF_ERROR_SOC_POWER_OFF_SHOULD_NOT_RETURN: return "NRF_ERROR_SOC_POWER_OFF_SHOULD_NOT_RETURN";
		case NRF_ERROR_SOC_RAND_NOT_ENOUGH_VALUES: return "NRF_ERROR_SOC_RAND_NOT_ENOUGH_VALUES";
		case NRF_ERROR_SOC_PPI_INVALID_CHANNEL: return "NRF_ERROR_SOC_PPI_INVALID_CHANNEL";
		case NRF_ERROR_SOC_PPI_INVALID_GROUP: return "NRF_ERROR_SOC_PPI_INVALID_GROUP";
		case BLE_ERROR_NOT_ENABLED:  // 0x3001
			return "BLE_ERROR_NOT_ENABLED";
		case BLE_ERROR_INVALID_CONN_HANDLE:  // 0x3002
			return "BLE_ERROR_INALID_CONN_HANDLE";
		case BLE_ERROR_INVALID_ATTR_HANDLE:  // 0x3003
			return "BLE_ERROR_INVALID_ATTR_HANDLE";
		case BLE_ERROR_INVALID_ADV_HANDLE:  // 0x3004
			return "BLE_ERROR_INVALID_ADV_HANDLE";
		case BLE_ERROR_INVALID_ROLE:  // 0x3005
			return "BLE_ERROR_INVALID_ROLE";
		case BLE_ERROR_BLOCKED_BY_OTHER_LINKS:  // 0x3006
			return "BLE_ERROR_BLOCKED_BY_OTHER_LINKS";
		default: return "Unknown";
	}
}

constexpr const char* nordicFDSTypeName(uint32_t nordic_type) {
	switch (nordic_type) {
			//		case FDS_SUCCESS:
			//			return "FDS_SUCCESS";
		case FDS_ERR_OPERATION_TIMEOUT: return "FDS_ERR_OPERATION_TIMEOUT";
		case FDS_ERR_NOT_INITIALIZED: return "FDS_ERR_NOT_INITIALIZED";
		case FDS_ERR_UNALIGNED_ADDR: return "FDS_ERR_UNALIGNED_ADDR";
		case FDS_ERR_INVALID_ARG: return "FDS_ERR_INVALID_ARG";
		case FDS_ERR_NULL_ARG: return "FDS_ERR_NULL_ARG";
		case FDS_ERR_NO_OPEN_RECORDS: return "FDS_ERR_NO_OPEN_RECORDS";
		case FDS_ERR_NO_SPACE_IN_FLASH: return "FDS_ERR_NO_SPACE_IN_FLASH";
		case FDS_ERR_NO_SPACE_IN_QUEUES: return "FDS_ERR_NO_SPACE_IN_QUEUES";
		case FDS_ERR_RECORD_TOO_LARGE: return "FDS_ERR_RECORD_TOO_LARGE";
		case FDS_ERR_NOT_FOUND: return "FDS_ERR_NOT_FOUND";
		case FDS_ERR_NO_PAGES: return "FDS_ERR_NO_PAGES";
		case FDS_ERR_USER_LIMIT_REACHED: return "FDS_ERR_USER_LIMIT_REACHED";
		case FDS_ERR_CRC_CHECK_FAILED: return "FDS_ERR_CRC_CHECK_FAILED";
		case FDS_ERR_BUSY: return "FDS_ERR_BUSY";
		case FDS_ERR_INTERNAL: return "FDS_ERR_INTERNAL";
		default: return "Unknown";
	}
}

constexpr const char* nordicFDSEventTypeName(uint32_t nordic_type) {
	switch (nordic_type) {
		case FDS_EVT_INIT: return "FDS_EVT_INIT";
		case FDS_EVT_WRITE: return "FDS_EVT_WRITE";
		case FDS_EVT_UPDATE: return "FDS_EVT_UPDATE";
		case FDS_EVT_DEL_RECORD: return "FDS_EVT_DEL_RECORD";
		case FDS_EVT_DEL_FILE: return "FDS_EVT_DEL_FILE";
		case FDS_EVT_GC: return "FDS_EVT_GC";
		default: return "Unknown";
	}
}

constexpr const char* nordicEventTypeName(uint32_t nordic_type) {
	switch (nordic_type) {
		case BLE_GAP_EVT_CONNECTED: return "BLE_GAP_EVT_CONNECTED";
		case BLE_EVT_USER_MEM_REQUEST: return "BLE_EVT_USER_MEM_REQUEST";
		case BLE_EVT_USER_MEM_RELEASE: return "BLE_EVT_USER_MEM_RELEASE";
		case BLE_GAP_EVT_DISCONNECTED: return "BLE_GAP_EVT_DISCONNECTED";
		case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST: return "BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST";
		case BLE_GATTS_EVT_TIMEOUT: return "BLE_GATTS_EVT_TIMEOUT";
		case BLE_GAP_EVT_RSSI_CHANGED: return "BLE_GAP_EVT_RSSI_CHANGED";
		case BLE_GATTS_EVT_WRITE: return "BLE_GATTS_EVT_WRITE";
		case BLE_GATTS_EVT_HVC: return "BLE_GATTS_EVT_HVC";
		case BLE_GATTS_EVT_SYS_ATTR_MISSING: return "BLE_GATTS_EVT_SYS_ATTR_MISSING";
		case BLE_GAP_EVT_PASSKEY_DISPLAY: return "BLE_GAP_EVT_PASSKEY_DISPLAY";
		case BLE_GAP_EVT_ADV_REPORT: return "BLE_GAP_EVT_ADV_REPORT";
		case BLE_GAP_EVT_TIMEOUT: return "BLE_GAP_EVT_TIMEOUT";
		case BLE_GATTS_EVT_HVN_TX_COMPLETE: return "BLE_GATTS_EVT_HVN_TX_COMPLETE";
		default: return "Unknown";
	}
}

////! called by soft device when you pass bad parameters, etc.
// void app_error_handler (uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name) {
//	volatile uint32_t error __attribute__((unused)) = error_code;
//	volatile uint16_t line __attribute__((unused)) = line_num;
//	volatile const uint8_t* file __attribute__((unused)) = p_file_name;
//
//	const char * str_error __attribute__((unused)) = NordicTypeName(error);
//	LOGe("%s", str_error);
//	LOGf("FATAL ERROR 0x%X, at %s:%d", error, file, line);
//
//	__asm("BKPT");
//	while (1) {}
//}
//
////! Called by softdevice
// void app_error_handler_bare(ret_code_t error_code)
//{
//	volatile uint32_t error __attribute__((unused)) = error_code;
//	volatile uint16_t line __attribute__((unused)) = 0;
//	volatile const uint8_t* file __attribute__((unused)) = NULL;
//
//	const char * str_error __attribute__((unused)) = NordicTypeName(error);
//	LOGe("%s", str_error);
//	LOGf("FATAL ERROR 0x%X", error);
//
//	__asm("BKPT");
//	while (1) {}
//}
//
////called by NRF SDK when it has an internal error.
// void assert_nrf_callback (uint16_t line_num, const uint8_t *file_name) {
//	volatile uint16_t line __attribute__((unused)) = line_num;
//	volatile const uint8_t* file __attribute__((unused)) = file_name;
//
//	LOGf("FATAL ERROR at %s:%d", file, line);
//
//	__asm("BKPT");
//	while (1) {}
//}
//
///*
//
////called by soft device when it has an internal error.
// void softdevice_assertion_handler(uint32_t pc, uint16_t line_num, const uint8_t * file_name) {
//	volatile uint16_t line __attribute__((unused)) = line_num;
//	volatile const uint8_t* file __attribute__((unused)) = file_name;
//	__asm("BKPT");
//	while (1) {}
//}
//
//*/
//
//#include <app_error.h>
//
// void mesh_assertion_handler(uint32_t pc)
//{
//    assert_info_t assert_info =
//    {
//        .line_num    = 0,
//        .p_file_name = (uint8_t *)"",
//    };
//#if NRF_SD_BLE_API_VERSION == 1
//    app_error_handler(NRF_FAULT_ID_SDK_ASSERT, pc, (const uint8_t *) "error");
//#elif NRF_SD_BLE_API_VERSION >= 2
//    app_error_fault_handler(NRF_FAULT_ID_SDK_ASSERT, pc, (uint32_t)(&assert_info));
//#endif
//
//    UNUSED_VARIABLE(assert_info);
//}
//
// void app_error_save_and_stop(uint32_t id, uint32_t pc, uint32_t info)
//{
//    /* static error variables - in order to prevent removal by optimizers */
//    static volatile struct
//    {
//        uint32_t        fault_id;
//        uint32_t        pc;
//        uint32_t        error_info;
//        assert_info_t * p_assert_info;
//        error_info_t  * p_error_info;
//        ret_code_t      err_code;
//        uint32_t        line_num;
//        const uint8_t * p_file_name;
//    } m_error_data = {0};
//
//    // The following variable helps Keil keep the call stack visible, in addition, it can be set to
//    // 0 in the debugger to continue executing code after the error check.
//    volatile bool loop = true;
//    UNUSED_VARIABLE(loop);
//
//    m_error_data.fault_id   = id;
//    m_error_data.pc         = pc;
//    m_error_data.error_info = info;
//
//    switch (id)
//    {
//        case NRF_FAULT_ID_SDK_ASSERT:
//            m_error_data.p_assert_info = (assert_info_t *)info;
//            m_error_data.line_num      = m_error_data.p_assert_info->line_num;
//            m_error_data.p_file_name   = m_error_data.p_assert_info->p_file_name;
//            break;
//
//        case NRF_FAULT_ID_SDK_ERROR:
//            m_error_data.p_error_info = (error_info_t *)info;
//            m_error_data.err_code     = m_error_data.p_error_info->err_code;
//            m_error_data.line_num     = m_error_data.p_error_info->line_num;
//            m_error_data.p_file_name  = m_error_data.p_error_info->p_file_name;
//            break;
//    }
//
//	LOGf("FATAL ERROR id=%u pc=u line=%u file=%s code=%u", m_error_data.fault_id, m_error_data.pc,
//m_error_data.line_num, m_error_data.p_file_name, m_error_data.err_code);
////	NRF_LOG_ERROR("FATAL ERROR id=%u pc=u line=%u file=%s code=%u", m_error_data.fault_id, m_error_data.pc,
///m_error_data.line_num, m_error_data.p_file_name, m_error_data.err_code);
//
//    UNUSED_VARIABLE(m_error_data);
//
//    // If printing is disrupted, remove the irq calls, or set the loop variable to 0 in the debugger.
//    __disable_irq();
//    while (loop);
//
//    __enable_irq();
//}
