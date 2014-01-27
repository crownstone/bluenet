

#include "HuffmanImage.h"

#include <algorithm>

void HuffmanImage::draw(RenderContext& buf) {
    HuffmanImageDecoder* dec = (HuffmanImageDecoder*)(pool_registry.get()->ptr(buf.state)); //buf.context[this]; // TODO: need multiple contexts per render
    if (!dec) {
        if (buf.state_owner) {
            // need allow multiple state objects in RC.
            NRF51_CRASH("Already have state in RenderContext. FIXME");
        }
        dec = new(buf.pool) HuffmanImageDecoder(this);
        buf.state = pool_registry.get()->off(dec);
        buf.state_owner = this;
    }

    dec->draw(buf);

}

void HuffmanImage::put(const HuffmanImageMessage& data) {
    numSegments = data.numMessages;

    HuffmanImageSegment * newSegment = new (getPool()) HuffmanImageSegment();
    pptr<HuffmanImageSegment> newPPtr = newSegment;
    if ( last == 0 ) {
        first = newPPtr;
    } else {
        last->next = newPPtr;
    }
    last = newPPtr;
    *newSegment = data;
    newSegment->next = NULL;
}

void HuffmanImage::put(const HuffmanCodebookMessage& data) {
    Pool * pool = getPool();
    HuffmanCodebookSegment * newSegment = new (pool) HuffmanCodebookSegment();
    pptr<HuffmanCodebookSegment> bookNode = codebook;
    if ( bookNode == 0 ) { codebook = newSegment; }
    else {
        while ( bookNode->next != 0 ) { bookNode = bookNode->next; }
        bookNode->next = newSegment;
    }
    *newSegment = data;
}


void HuffmanImage::cleanup(RenderContext& ctx, void *mem) {
    pptr<HuffmanImageDecoder> dec;
    dec.set(ctx.state);
    if (dec) {
        dec->~HuffmanImageDecoder();
        ctx.pool->release(sizeof(HuffmanImageDecoder), (HuffmanImageDecoder*)dec);
        ctx.state = 0;
    }
}



void HuffmanImageDecoder::reset() {
    msg = image->first;
    init();

}

void HuffmanImageDecoder::next() {
    msg = msg->next;
    if (msg) init();
}

void HuffmanImageDecoder::init() {
    bitIndex = 0;
    byteIndex = 0;
    x = (msg->byteOffset % 33) * 8;
    y = (msg->byteOffset / 33);
}

int counter = 0;
void HuffmanImageDecoder::draw(RenderContext& ctx) {
    if ( ! msg ) { return; }
    pptr<HuffmanCodebookSegment> codebookRoot = image->codebook;

    if (ctx.start_y  < y || ctx.start_x < x) {
        // we've gone too far into the decoded image data to fulfill this request.
        // lets back up to beginning and try again.
        // the client should try to avoid triggering this case.
        reset();
    }

    if ( msg->byteOffset == HuffmanImageSegment::invalid ) {
        NRF51_CRASH("invalid");
    }

    uint8_t* buf = ctx.buffer;

    pptr<HuffmanCodebookSegment> currentBook = codebookRoot;
    uint8_t bookSubIndex = 0;

    while ( msg ) {
        while( byteIndex < HuffmanImageSegment::message_length ) {
            uint8_t byte = msg->data[byteIndex];
            while ( bitIndex < 8 ){
                //I can get replace these trinary statements with bitwise operations if I can be assured that
                //<code> (byte & (1 << bitIndex) != 0 </code> always evaluates to either 1 or 0. I think that
                //it is only guaranteed that the result will be 0 if false, and not 0 otherwise, though.

                uint8_t bit = byte & (1 << bitIndex);
                uint8_t leafMask = bit ? 0x01 : 0x02;
                uint8_t bookByte = currentBook->data[bookSubIndex];
                uint8_t isLeaf = bookByte & leafMask;
                uint8_t jump = bit ? 1u : (((bookByte >> 2) << 1)+2);

                bitIndex += 1;

                bookSubIndex += jump;
                while ( bookSubIndex >= HuffmanCodebookSegment::data_length ) {
                    bookSubIndex -= HuffmanCodebookSegment::data_length;
                    currentBook = currentBook->next;

                    if ( ! currentBook ) {
                        bitIndex = 8;
                        byteIndex = HuffmanImageSegment::message_length;
                        isLeaf = 0;
                        bookSubIndex = 0;
                        currentBook = codebookRoot;
                    }
                }

                if ( isLeaf ) {
                    *buf = ~(currentBook->data[bookSubIndex]);
                    ++buf;

                    x += 8;

                    currentBook = codebookRoot;
                    bookSubIndex = 0;

                    if ( x >= ctx.start_x + ctx.width ) {
                        x = 0;
                        ++y;
                        if ( y >= ctx.start_y + ctx.height ) {
                            return;
                        }
                        buf += ctx.stride;
                    }
                }
            }
            bitIndex = 0;
            byteIndex ++;
        }

        bookSubIndex = 0;
        currentBook = codebookRoot;

        next();
    }
}

const uint16_t base64_chars_len = 65;
const char* Base64Decoder::base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz"
                "0123456789+/";

HuffmanImageMessageFactory& HuffmanImageMessageFactory::add(uint8_t& a) {
    message.raw[offset++] = a;
    return *this;
}
HuffmanImageMessageFactory& HuffmanImageMessageFactory::put() {
    image.put(message);
    message = HuffmanImageMessage();
    offset = 0;
    return *this;
}


HuffmanImageMessageFactory& HuffmanImageMessageFactory::add(const char** const messages, int len) {
    for(int i = 0; i < len; ++i) {
        Base64Decoder::base64_decode(messages[i], strlen(messages[i]),message.raw);
        put();
    }
    return *this;
}



HuffmanCodebookMessageFactory& HuffmanCodebookMessageFactory::add(uint8_t& a) {
    message.raw[offset++] = a;
    return *this;
}
HuffmanCodebookMessageFactory& HuffmanCodebookMessageFactory::put() {
    image.put(message);
    message = HuffmanCodebookMessage();
    offset = 0;
    return *this;
}


HuffmanCodebookMessageFactory& HuffmanCodebookMessageFactory::add(const char** const messages, int len) {
    for(int i = 0; i < len; ++i) {
        Base64Decoder::base64_decode(messages[i], strlen(messages[i]),message.raw);
        put();
    }
    return *this;
}





void Base64Decoder::base64_decode(const char* encoded_string, int in_len, uint8_t * buffer) {
    int bufferIndex = 0;

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
                buffer[bufferIndex++] = char_array_3[i];
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
            buffer[bufferIndex++] = (char_array_3[j]);
        }
    }
}

