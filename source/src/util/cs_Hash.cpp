/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 30, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <util/cs_Hash.h>
#include <util/cs_Math.h>
#include <drivers/cs_Serial.h>

// based on source(30-10-2019): https://en.wikipedia.org/wiki/Fletcher%27s_checksum
// adjusted to handle uint8_t arrays by padding with 0x00
uint32_t Fletcher(uint8_t *data_8, const size_t len) {
        uint16_t* data_16 = reinterpret_cast<uint16_t*>(data_8);
        uint32_t c0 = 0;
        uint32_t c1 = 0;

        bool needs_padding = len % 2 !=0;
        size_t loop_len = len/2; // (this rounds down when necessary)

        // 360 is chosen such that c0 and c1 are accumulated without overflow.
        // Delaying the % operator until the last possible moment reduced the
        // amount of calls to it.
        while(loop_len > 0) {
                // subtract 1 from length if it isn't even to prevent overflow. 
                size_t blocklen = CsMath::min(360u, loop_len);

                for (size_t i = 0; i < blocklen; ++i) {
                        LOGd("data: %x",*data_16);
                        // c0 += data_8[0] <<8 | data_8[1];
                        // data_8 += 2;

                        c0 += *data_16++;
                        c1 += c0;
                        LOGd(". %x %x", c0,c1);
                }
                c0 = c0 % 0xffff;
                c1 = c1 % 0xffff;

                loop_len -= blocklen;
                LOGd(">");
        }

        if(needs_padding){
            // intentionally skipped the last byte of data in the loop above
            // padd with 0x00 to finish the has.            
            LOGd("data %x",data_8[len-1] | 0x00 << 8);
            c0 += data_8[len-1] | 0x00  << 8;
            // c0 += *data_8 << 8 | 0x00;

            c1 += c0;
            
            c0 = c0 % 65535;
            c1 = c1 % 65535;
            
            LOGd("! %x %x", c0,c1);
        }

        return (c1 << 16 | c0);
}