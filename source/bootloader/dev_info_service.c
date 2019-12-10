/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: 30 May, 2019
 * Triple-license: LGPLv3+, Apache License, and/or MIT
 */
#include <string.h>
//#include "ble_gap.h"
//#include "ble_srv_common.h"
#include "ble_dis.h"
#include "app_error.h"

#include "cfg/cs_HardwareVersions.h"
#include "cs_BootloaderConfig.h"

/**
 * Adds the device information service.
 */
void dev_info_service_init(void)
{
    ble_dis_init_t dis_init;
    memset(&dis_init, 0, sizeof(dis_init));
    ble_srv_ascii_to_utf8(&dis_init.hw_rev_str, (char*)get_hardware_version());
    ble_srv_ascii_to_utf8(&dis_init.fw_rev_str, (char*)g_BOOTLOADER_VERSION);
    dis_init.dis_char_rd_sec = SEC_OPEN;
    uint32_t err_code = ble_dis_init(&dis_init);
    APP_ERROR_CHECK(err_code);
}
