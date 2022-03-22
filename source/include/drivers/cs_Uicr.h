/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Mar 17, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <protocol/cs_UicrPacket.h>
#include <protocol/cs_Typedefs.h>

cs_ret_code_t getUicr(cs_uicr_data_t* uicrData);
cs_ret_code_t setUicr(const cs_uicr_data_t* uicrData);

#ifdef __cplusplus
}
#endif
