
#include "RLEImage.h"

#include <algorithm>

const coord_t RLEImageSegment::invalid = std::numeric_limits<coord_t>::max();


void RLEImage::draw(RenderContext& buf) {
    RLEImageDecoder* dec = (RLEImageDecoder*)(pool_registry.get()->ptr(buf.state)); //buf.context[this]; // TODO: need multiple contexts per render
    if (!dec) {
        if (buf.state_owner) {
            // need allow multiple state objects in RC.
            NRF51_CRASH("Already have state in RenderContext. FIXME");
        }
        dec = new(buf.pool) RLEImageDecoder(this);
        buf.state = pool_registry.get()->off(dec);
        buf.state_owner = this;
    }

    dec->draw(buf);

}

void RLEImage::put(const RLEImageMessage& data) {
    uint8_t msg_seq = data.sequence;

    if (std::max(msg_seq, seq) - std::min(msg_seq, seq) > max_missed_messages) {
        seq = msg_seq - 1;
    }

    // preallocate any messages that we haven't received yet based on sequence #.
    while (seq != msg_seq) { // don't use < because sequence number could wrap.
        RLEImageSegment* msg = new (getPool()) RLEImageSegment(); // allocate from pool.
        if (!last) {
            last = first = msg;
        } else {
            last->next = msg;
            msg->prev = last;
            last = msg;
        }
        seq++;
    }

    RLEImageSegment* msg = last;
    uint8_t s = seq;

    // now, back up to the one we just received. (Note, this doesn't happen if the above while loop occurred.)
    while(s-- != msg_seq) {
        msg = msg->prev;
    }

    *msg = data;
}

void RLEImage::cleanup(RenderContext& ctx, void *mem) {
    pptr<RLEImageDecoder> dec;
    dec.set(ctx.state);
    if (dec) {
        dec->~RLEImageDecoder();
        ctx.pool->release(sizeof(RLEImageDecoder), (RLEImageDecoder*)dec);
        ctx.state = 0;
    }
}

void RLEImageDecoder::reset() {
    msg = image->first;
    init();
    outbyte = 0;
}

void RLEImageDecoder::next() {
    msg = msg->next;
    if (msg) init();
}

void RLEImageDecoder::init() {
    if (first) {
        first = 0;
    } else {
        if (x != msg->x) {
           // NRF51_CRASH("invalid");
        }
    }
    bit = msg->data[0] & 1; // is first pixel a one or zero.
    i = 0;
    j = 1;



    x = msg->x;
    y = msg->y;
    pixels = 0;
}

void RLEImageDecoder::draw(RenderContext& ctx) {


    mask = 0; // ? need mask stride?
    
    uint8_t* buf = ctx.buffer;
    outbyte = *buf;

    if (ctx.start_y  < y || ctx.start_x < x) {
        // we've gone too far into the decoded image data to fulfill this request.
        // lets back up to beginning and try again.
        // the client should try to avoid triggering this case.
        reset();
    }

    // Little endian run length encoded values consist of a single bit indicating whether the first run is
    // white or black.  This is followed by Elias-coded run counts.  The count consists of a number, n, of 0's,
    // followed by a binary coded decimal of n binary digits.  This decimal
    // is the number of pixels in the run.  Therefore a series of 1's indicates alternating pixel values.
    // See http://en.wikipedia.org/wiki/Elias_gamma_coding for details.

    bool in_count = false;  // state machine: are we in the zero portion (false) or the run count portion (true).

    // first, complete any of the left over pixels from the previous run, and if that completely satisfies the request, return.
    if (pixels > 0) {
        if (drawPixels(ctx, &buf)) {
            if (y != ctx.start_y + ctx.height) {
                NRF51_CRASH("feh");
            }
            return;
        } else {
            bit = !bit;
            j++;
            if (j == 8) {
                i++;
                j = 0;
            }
        }
    }

    while(msg) {

        if (msg->x == RLEImageSegment::invalid || (msg->next && (msg->next->x < ctx.start_x && msg->next->y < ctx.start_y))) {
            NRF51_CRASH("invalid");
            next();
            continue;  // skip missing messages and messages that are before where we're asked to draw.
        }

        uint32_t bits = 0;
        uint8_t bitsFound = 0;

        // now step through the bytes of the message, interpreting them as run lengths as described above.
        for( ; i < RLEImageSegment::message_length; ++i) {
            uint8_t byte = msg->data[i];
            for(; j < 8; ++j) {
                if (!in_count) {
                    bits++;
                    if (byte & (1 << j)) {
                        in_count = true;
                        bitsFound = bits;
                    }
                }
                if (in_count) { // happens immediately in the case of a run count of 1.
                    pixels += ((byte & (1 << j)) ? 1 : 0) << (bits-1);
                    if (--bits == 0) {
                        if (pixels > image->getHeight() * image->getWidth() || pixels == 0) {
                            NRF51_CRASH("invalid");
                        }
                        bool done = drawPixels(ctx, &buf);
                        if (done) {
                            return;
                        } else {
                            bits = 0;
                            bit = !bit;
                            in_count = false;
                        }
                    }
                }
            }
            j = 0;
        }
        // last byte(s) will be padded with zeros so that we simply walk out of the loop.

        next();
    }
}

#ifdef USE_RENDER_CONTEXT
bool RLEImageDecoder::drawPixels(RenderContext& ctx, uint8_t** bufptr) {

    if (pixels == 0) return false;
    outbyte = **bufptr;
    while(pixels > 0 && (y < ctx.start_y + ctx.height) && (x < ctx.start_x + ctx.width)) {

        if (x >= ctx.start_x && y >= ctx.start_y) {
            pixels--;
            ctx.setPixel(x, y, bit);
        }
        if (++x == ctx.start_x + ctx.width) {
            x = 0;
            y++;
        }
    }
    return y == ctx.start_y + ctx.height;
}

#else /* USE_RENDER_CONTEXT */

bool RLEImageDecoder::drawPixels(RenderContext& ctx, uint8_t** bufptr) {

    if (pixels == 0) return false;
    outbyte = **bufptr;
    while(pixels > 0 && (y < ctx.start_y + ctx.height) && (x < ctx.start_x + ctx.width)) {

        if (x >= ctx.start_x && y >= ctx.start_y) {
            if (bit) {
                outbyte |= 1 << mask;
            } else {
                outbyte &= ~ (1 << mask);
            }
            pixels--;
           // total_pixels++;
            mask++;
            if (mask == 8) {
                **bufptr = outbyte;
                if (x != ctx.start_x + ctx.width - 1) {
                    (*bufptr)++;
                    if (!ctx.check(*bufptr)) {
                        NRF51_CRASH("Render buffer exceeded while advancing columns in RLEImageDecoder.");
                    }
                    outbyte = **bufptr;
                }
                mask = 0;
            }
        } else {
            NRF51_CRASH("Skipping.");
        }
        if (++x == ctx.start_x + ctx.width) {
            **bufptr = outbyte;
            x = 0;
            mask = 0;
            y++;
            if (y == ctx.start_y + ctx.height) {
                return true;
            }

            *bufptr += ctx.stride;
            if (!ctx.check(*bufptr)) {
                NRF51_CRASH("Render buffer exceeded while advancing rows in RLEImageDecoder.");
            }
            outbyte = **bufptr;

        }
        if (msg->next && x > msg->next->x && y > msg->next->y) {
            NRF51_CRASH("Surpassed next message in RLEImageDecoder.");
        }
    }
    return y == ctx.start_y + ctx.height;
}

#endif /* USE_RENDER_CONTEXT */

const uint16_t base64_chars_len = 65;
const char* RLEImageMessageFactory::base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

RLEImageMessageFactory& RLEImageMessageFactory::add(uint8_t& a) {
    message.raw[offset++] = a;
    return *this;
}
RLEImageMessageFactory& RLEImageMessageFactory::put() {
    image.put(message);
    message = RLEImageMessage();
    offset = 0;
    return *this;
}


RLEImageMessageFactory& RLEImageMessageFactory::add(const char** const messages, int len) {
    for(int i = 0; i < len; ++i) {
        base64_decode(messages[i], strlen(messages[i]));
        put();
    }
    return *this;
}

inline bool RLEImageMessageFactory::is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

void RLEImageMessageFactory::base64_decode(const char* encoded_string, int in_len) {
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i ==4) {
            for (i = 0; i <4; i++)
                char_array_4[i] = std::find(base64_chars, base64_chars+base64_chars_len, char_array_4[i]) - base64_chars;

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++) {
                add(char_array_3[i]);
            }
            i = 0;
        }
    }

    if (i) {
        for (j = i; j <4; j++)
            char_array_4[j] = 0;

        for (j = 0; j <4; j++)
            char_array_4[j] = std::find(base64_chars, base64_chars+base64_chars_len, char_array_4[j]) - base64_chars;

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; (j < i - 1); j++) {
            add(char_array_3[j]);
        }
    }
}

