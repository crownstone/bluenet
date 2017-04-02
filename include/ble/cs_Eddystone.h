/*
 * Authors: Crownstone Team
 * Copyright: Crownstone B.V.
 * Date: Jun 3, 2016
 * License: LGPLv3+, Apache, or MIT, your choice
 */

#pragma once

#include <ble/cs_UUID.h>

#include <drivers/cs_Serial.h>

typedef struct
{
    uint8_t adv_frame[BLE_GAP_ADV_MAX_SIZE];
    uint8_t adv_len;
} edstn_frame_t;

//namespace BLEpp {

/** Implementation of the Eddystone specification.
 *
 */
class Eddystone {
	private:
	public:

    void advertising_init(void);
    void advertising_start(void);

    void init_url_frame_buffer();
    void init_tlm_frame_buffer();
    void init_uid_frame_buffer();

    uint32_t eddystone_set_adv_data(uint32_t frame_index);


        uint32_t eddystone_head_encode(uint8_t *p_encoded_data,
                uint8_t frame_type,
                uint8_t *p_len);

        uint32_t eddystone_uint32(uint8_t *p_encoded_data,
                uint8_t *p_len,
                uint32_t val);

        uint32_t eddystone_uint16(uint8_t *p_encoded_data,
                uint8_t *p_len,
                uint16_t val);

};

//} //! end namespace
