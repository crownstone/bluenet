#ifndef _HuffmanImage_h
#define _HuffmanImage_h

#include "Pool.h"

#include "DisplayObject.h"

struct HuffmanImageMessage {
    typedef uint8_t offset_t;

    static const uint32_t header_length = 3;
    static const uint32_t data_length = 17;
    static const uint32_t len = 20;

    uint8_t raw[0];
    uint16_t byteOffset;
    uint8_t numMessages; //Corresponds to the next pointer in a segment
    uint8_t  data[data_length];

    HuffmanImageMessage() : HuffmanImageMessage(0, 0) {}

    HuffmanImageMessage(const uint8_t* d, size_t size) {
        if (size > len) {
            NRF51_CRASH("Message too big.");
        }
        uint32_t i = 0;
        for(; i < len && i < size; ++i) {
            raw[i] = d[i];
        }
        for(; i < len; ++i) {
            raw[i] = 0;
        }
    }

};

struct HuffmanImageSegment { // pooled 20 bytes.
    static const uint32_t header_length = 3;
    static const uint32_t message_length = 17;
    static const uint32_t len = 20;
    static const uint16_t invalid = (uint16_t) -1;

    uint16_t byteOffset;
    pptr<HuffmanImageSegment> next;
    uint8_t data[message_length];

    HuffmanImageSegment() : byteOffset(invalid), next(0) {
    }

    HuffmanImageSegment(const HuffmanImageMessage& rhs) : /* do not initialize prev/next! */ byteOffset(rhs.byteOffset) {
        next = 0;
        copy(rhs);
    }

    void* operator new(size_t sz, Pool* p) {
        return p->allocate(sz);
    }

    ~HuffmanImageSegment() {
        byteOffset = 0;
        for(uint32_t i = 0; i < message_length; ++i) {
            data[i] = 0;
        }
    }

    HuffmanImageSegment& operator=(const HuffmanImageMessage& rhs) {
        //do not initialize prev/next!
        byteOffset = rhs.byteOffset;
        copy(rhs);
        return *this;
    }

private:
    void copy(const HuffmanImageMessage& rhs) {
        for(uint32_t i = 0; i < message_length; ++i) {
            data[i] = rhs.data[i];
        }
    }

};

struct HuffmanCodebookMessage {
    static const uint32_t header_length = 1;
    static const uint32_t data_length = 19;
    static const uint32_t len = header_length + data_length;

    uint8_t raw[0];
    uint8_t unused; //corresponds to next pointer
    uint8_t data[data_length];

    HuffmanCodebookMessage() : HuffmanCodebookMessage(0, 0) {}

    HuffmanCodebookMessage(const uint8_t* d, size_t size) {
        if (size > len) {
            NRF51_CRASH("Message too big.");
        }
        uint32_t i = 0;
        for(; i < len && i < size; ++i) {
            raw[i] = d[i];
        }
        for(; i < len; ++i) {
            raw[i] = 0;
        }
    }
};



struct HuffmanCodebookSegment {
    static const uint32_t header_length = 1;
    static const uint32_t data_length = 19;

    pptr<HuffmanCodebookSegment> next;
    uint8_t data[data_length];

    void* operator new(size_t sz, Pool* p) {
        return p->allocate(sz);
    }

    HuffmanCodebookSegment& operator=(const HuffmanCodebookMessage& rhs) {
        //do not initialize prev/next!
        copy(rhs);
        return *this;
    }

private:
    void copy(const HuffmanCodebookMessage& rhs) {
        for(uint32_t i = 0; i < data_length; ++i) {
            data[i] = rhs.data[i];
        }
    }
};


class HuffmanImageDecoder;

/** A Huffman Encoded image, stored as a series of 20 byte messages.  Tolerates out-of-order delivery of messages 
* (so long as the codebook has been delivered reliably) and allocates message storage from a Pool.
*
* This image can be read out incrementally using HuffmanDecoder.
*/
class HuffmanImage : public RectDisplayObject {
    // tolerate up to this many missing messages.
    static const uint8_t max_missed_messages = 5;

    friend class HuffmanImageDecoder;

    pptr<HuffmanCodebookSegment> codebook;
    pptr<HuffmanImageSegment> first;
    pptr<HuffmanImageSegment> last;
    int8_t numSegments;

    coord_t _width;
    coord_t _height;
public:
    HuffmanImage(DisplayObject* parent, coord_t x, coord_t y, coord_t w, coord_t h) : RectDisplayObject(parent, x, y, true, 1), first(0), last(0),
        numSegments(-1),
        _width(w),
        _height(h)
    {}



    virtual ~HuffmanImage() {
        _width = _height = 0;
        while(first) {
            HuffmanImageSegment* tmp = first->next;
            first->~HuffmanImageSegment();
            getPool()->release(HuffmanImageSegment::len, first);
            first = tmp;
        }
    }

    virtual size_t getSize() const {
        return sizeof(HuffmanImage);
    }

    virtual coord_t getWidth () { return _width; }
    virtual coord_t getHeight () { return _height; }

    void* operator new(size_t sz, Pool* p) {
        return p->allocate(sz);
    }

    void put(const HuffmanImageMessage& data);
    void put(const HuffmanCodebookMessage& data);

    virtual void draw(RenderContext& buf);

    virtual bool intersects(int16_t origin_x, int16_t origin_y, int16_t width, int16_t height) {
        return true; // TODO.
    }

    virtual void cleanup(RenderContext& ctx, void *mem);
};


/** A state machine for decoding a series of run length encoded image messages.  Can draw an arbitrary potion of
* the image by advancing through messages until the requested parts of the image are found.  Caches enough state
* to make sequentially drawing the image faster.  This way we can render lines at a time without having to re-
* read a bunch of the image messages.
*/
class HuffmanImageDecoder {  // this is designed to be exactly 20 bytes so that it fits in a Pool block during rendering.
    pptr<HuffmanImage> image;
    pptr<HuffmanImageSegment> msg;

    uint16_t x,y;
    int totalBytes;
    uint16_t byteIndex, bitIndex;
public:
    HuffmanImageDecoder(HuffmanImage* img) : image(img) {
        reset();
    }

    void* operator new(size_t sz, Pool* p) {
        return p->allocate(sz);
    }

    void reset();

    void next();

    void init();

    void draw(RenderContext& ctx);
    bool drawPixel(RenderContext& ctx, uint8_t** bufptr, uint8_t outbyte);
};


struct HuffmanImageMessageFactory {

    HuffmanImage& image;

    uint8_t offset;

    HuffmanImageMessage message;


    HuffmanImageMessageFactory(HuffmanImage& img) : image(img), offset(0), message() { }
    HuffmanImageMessageFactory& add(uint8_t& a);
    HuffmanImageMessageFactory& put();
    /** Adds a series of base64 encoded run length messages, the same format as output by the usvg.js code. This
    * is useful primarily for testing. */
    HuffmanImageMessageFactory& add(const char** const messages, int len);
};

struct HuffmanCodebookMessageFactory {

    HuffmanImage& image;

    uint8_t offset;

    HuffmanCodebookMessage message;


    HuffmanCodebookMessageFactory(HuffmanImage& img) : image(img), offset(0), message() { }
    HuffmanCodebookMessageFactory& add(uint8_t& a);
    HuffmanCodebookMessageFactory& put();
    /** Adds a series of base64 encoded run length messages, the same format as output by the usvg.js code. This
    * is useful primarily for testing. */
    HuffmanCodebookMessageFactory& add(const char** const messages, int len);
};

struct Base64Decoder {
    static void base64_decode(const char* encoded_string, int in_len, uint8_t * outputBuffer );
    static const inline bool is_base64(unsigned char c) {
        return (isalnum(c) || (c == '+') || (c == '/'));
    }
    static const char* base64_chars ;
};


#endif /* _HuffmanImage_h */