#ifndef _RLEImage_h
#define _RLEImage_h

#include "Pool.h"

#include "DisplayObject.h"

struct RLEImageMessage {
    typedef uint8_t offset_t;

    static const uint32_t header_length = 6;
    static const uint32_t data_length = 14;
    static const uint32_t len = 20;

    uint8_t raw[0];
    uint8_t command;
    uint8_t sequence;
    uint16_t x,y;
    uint8_t  data[data_length];

    RLEImageMessage() : RLEImageMessage(0, 0) {}

    RLEImageMessage(const uint8_t* d, size_t size) {
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


struct RLEImageSegment { // pooled 20 bytes.
    static const uint32_t header_length = 6;
    static const uint32_t message_length = 14;
    static const uint32_t len = 20;
    static const coord_t invalid;

    pptr<RLEImageSegment> prev;
    pptr<RLEImageSegment> next;
    coord_t x,y;
    uint8_t data[message_length];

    RLEImageSegment() : x(invalid){
    }

    RLEImageSegment(const RLEImageMessage& rhs) : /* do not initialize prev/next! */ x(rhs.x), y(rhs.y) {
        copy(rhs);
    }

    void* operator new(size_t sz, Pool* p) {
        return p->allocate(sz);
    }

    ~RLEImageSegment() {
        x = y = 0;
        for(uint32_t i = 0; i < message_length; ++i) {
            data[i] = 0;
        }
        prev = next = NULL;
    }

    RLEImageSegment& operator=(const RLEImageMessage& rhs) {
        //do not initialize prev/next!
        x = rhs.x;
        y = rhs.y;
        copy(rhs);
        return *this;
    }

private:
    void copy(const RLEImageMessage& rhs) {
        for(uint32_t i = 0; i < message_length; ++i) {
            data[i] = rhs.data[i];
        }
    }

};

class RLEImageDecoder;

/** A Run Length Encoded image, stored as a series of 20 byte messages.  Tolerates out-of-order delivery of messages and
* allocates message storage from a Pool.
*
* This image can be read out incrementally using RLEImageDecoder.
*/
class RLEImage : public RectDisplayObject {
    // tolerate up to this many missing messages.
    static const uint8_t max_missed_messages = 5;

    friend class RLEImageDecoder;
    typedef uint8_t seq_t;
    pptr<RLEImageSegment> first;
    pptr<RLEImageSegment> last;
    seq_t seq; // last sequence # we saw.

    coord_t _width;
    coord_t _height;
  public:
    RLEImage(DisplayObject* parent, coord_t x, coord_t y, coord_t w, coord_t h) : RectDisplayObject(parent, x, y, true, 1), first(0), last(0),
        seq(std::numeric_limits<seq_t>::max()),
        _width(w),
        _height(h)
    {}

    virtual coord_t getWidth () { return _width; }
    virtual coord_t getHeight () { return _height; }

    virtual ~RLEImage() {
        _width = _height = 0;
        while(first) {
            RLEImageSegment* tmp = first->next;
            first->~RLEImageSegment();
            getPool()->release(RLEImageSegment::len, first);
            first = tmp;
        }
    }

    virtual size_t getSize() const {
        return sizeof(RLEImage);
    }

    void* operator new(size_t sz, Pool* p) {
        return p->allocate(sz);
    }

    void put(const RLEImageMessage& data);

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
class RLEImageDecoder {  // this is designed to be exactly 20 bytes so that it fits in a Pool block during rendering.
    pptr<RLEImage> image;
    pptr<RLEImageSegment> msg;
    uint16_t x,y;
    uint32_t pixels;
    uint16_t i, j;
    uint8_t outbyte;
    uint8_t mask;
    bool bit;
    bool first;
  public:
    RLEImageDecoder(RLEImage* img) : image(img), first(1) {
        reset();
    }

    void* operator new(size_t sz, Pool* p) {
        return p->allocate(sz);
    }

    void reset();

    void next();

    void init();

    void draw(RenderContext& ctx);
    bool drawPixels(RenderContext& ctx, uint8_t** bufptr);

};


struct RLEImageMessageFactory {

    RLEImage& image;

    uint8_t offset;
    uint8_t sequence;

    RLEImageMessage message;


    RLEImageMessageFactory(RLEImage& img) : image(img), offset(0), sequence(0), message() { }
    RLEImageMessageFactory& add(uint8_t& a);
    RLEImageMessageFactory& put();
    /** Adds a series of base64 encoded run length messages, the same format as output by the usvg.js code. This
    * is useful primarily for testing. */
    RLEImageMessageFactory& add(const char** const messages, int len);


  protected:

    static const char* base64_chars ;
    inline bool is_base64(unsigned char c);

    void base64_decode(const char* encoded_string, int in_len);

};


#endif /* _RLEImage_h */