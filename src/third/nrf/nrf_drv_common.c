/* Copyright (c) 2015 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#include "nrf_drv_common.h"
#include "nrf_assert.h"
#include "nrf_error.h"

#include <drivers/cs_Serial.h>

#ifdef SOFTDEVICE_PRESENT
#include "nrf_soc.h"
#endif

void nrf_drv_common_irq_enable(IRQn_Type IRQn, uint8_t priority)
{

//	LOGi(">>> enable: %d", IRQn);

#ifdef SOFTDEVICE_PRESENT
	ASSERT((priority == NRF_APP_PRIORITY_LOW) || (priority == NRF_APP_PRIORITY_HIGH));

//	LOGi(">>> SOFTDEVICE_PRESENT: %d", IRQn);
	sd_nvic_SetPriority(IRQn, priority);
	sd_nvic_ClearPendingIRQ(IRQn);
	sd_nvic_EnableIRQ(IRQn);
#else
	NVIC_SetPriority(IRQn, priority);
	NVIC_ClearPendingIRQ(IRQn);
	NVIC_EnableIRQ(IRQn);
#endif

}
