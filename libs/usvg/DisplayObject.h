#ifndef _DisplayObject_h
#define _DisplayObject_h

#include <string>
//#include <unordered_map>
#include "function.h"

#include "vector_math.h"

#include "Pool.h"
#include "DisplayFwd.h"

using std::string;
using vmath::vec3;
using vmath::mat3;

typedef int16_t int_coord_t;

#define MATH_FORMAT MATH_FORMAT_INTEGER

#if MATH_FORMAT == MATH_FORMAT_INTEGER
// integer impl
typedef int16_t coord_s_t; // storage format.  must be 2 byte.
typedef int16_t coord_t; // calculation format.
typedef bool  color_t;


#elif MATH_FORMAT == MATH_FORMAT_FIXED

// fixed impl
typedef accum coord_s_t; // storage format.  must be 2 byte.
typedef short accum coord_t; // calculation format.
typedef bool  color_t;


#elif MATH_FORMAT == MATH_FORMAT_FIXED

// float implementation
typedef __fp16 coord_s_t; // storage format.  must be 2 byte.  __fp16 is a gcc ARM extension.
typedef float coord_t; // calculation format.
typedef bool  color_t;
*/
#else
#error "Please define MATH_FORMAT as MATH_FORMAT_INTEGER, MATH_FORMAT_FIXED, or MATH_FORMAT_FLOAT"
#endif


typedef vec3<coord_t> coord_vec_3_t;
typedef vec3<coord_s_t> coord_vec_3_s_t; // storage format. must be 6 byte.
typedef mat3<coord_t> coord_mat_3_t;
typedef mat3<coord_s_t> coord_mat_3_s_t; // storage format.  must be 18 byte.

struct rect {
    coord_t x,y, width, height;
    rect() : x(0), y(0), width(0), height(0) {}
    rect(coord_t _x, coord_t _y, coord_t _w, coord_t _h) : x(_x), y(_y), width(_w), height(_h) {}
    rect operator+(const rect& rhs) {
        return rect(std::min(x, rhs.x), std::min(y, rhs.y), std::max(width, rhs.width), std::max(height, rhs.height));
    }
    rect& operator+=(const rect& rhs) {
        *this = *this + rhs;
        return *this;
    }
};

/** State buffer for a rendering operation.  Because the entire display is too large to fit in memory, we render
* individual tiles and send them to the display as they are rendered.  This contains a pixel array that represents an
* arbitrary subregion of the (possibly) larger complete image we're rendering.  Also has a map that individual DisplayObjects
* can use to buffer state during rendering.
*
* Note: buffer is currently assumed to be a multiple of 4 bytes wide.
*/
struct RenderContext {

    static const bool debug = false;

    Pool* pool;

    coord_t start_x;
    coord_t start_y;
    coord_t width; // the width of the window we're drawing.
    coord_t height; // the height of the window we're drawing.
    uint16_t stride; // distance in the buffer between one row and the next (== 0 if width == buffer width).
    uint8_t* buffer;
    uint8_t mask;  // which bit at *buffer corresponds to start_x, start_y?
    // TODO does this also need a mask stride in case the stride isn't a multiple of 8 bits?

    uint8_t bit_depth;  //how many bits per pixel.
    uint32_t bd;
    uint32_t bdmask;

    coord_mat_3_t transform;

    //std::unordered_map<DisplayObject* , void*> context;
    uint8_t state;
    pptr<DisplayObject> state_owner;

    // a 4 byte chunk of buffer kept in a register:
    coord_t cx, cy;  // x and y position of bit 0 of chunk
    uint32_t chunk;
    bool dirty;


    RenderContext(Pool* p, uint16_t w, uint16_t h, uint8_t* b) : pool(p), start_x(0), start_y(0), width(w), height(h), stride(0),
         buffer(b), mask(0), bit_depth(1), bdmask(0), transform(0), state(0), cx(0), cy(0), dirty(false) {
        bd = 32U/bit_depth;
        uint32_t i = 31;
        while (!(bd & (1 << i))) i--; // clz
        bd = i;
        for(i = 0; i < bd; ++i) {
            bdmask |= (1 << i);
        }
        bdmask = ~bdmask;

        advance(0,0,b);
    }

    ~RenderContext();

    void advance(coord_t x, coord_t y, uint8_t* b) {
        start_x = x;
        start_y = y;
        buffer = b;
        cx = (coord_t)(x & bdmask);
        cy = y;
        if (buffer) chunk = *ptr();
        mask = 0;
        dirty = false;
    }

    void setPixel(coord_t x, coord_t y, color_t color) {
        load(x,y);

        if (color) { // TODO: other than 1-bit color depth.
            chunk |= 1 << (x - cx);
        } else {
            chunk &= ~(1 << (x - cx));
        }
        dirty = true;
    }

    void load(coord_t x, coord_t y) {
        if ((x & bdmask) != cx || y != cy) {
            if (debug && (x >= (start_x+width) || y >= (start_y+height) || x < start_x || y < start_y )) {
                NRF51_CRASH("Out of bounds.");
            }
            if (dirty) {
                *ptr() = chunk;
                dirty = false;
            }

            cy = y;
            cx = (coord_t)(x & bdmask);

            chunk = *ptr();
        }
    }

    uint32_t* ptr() {
        coord_t dy = cy - start_y;
        return (((uint32_t*)buffer) + ((dy * width + dy * stride + (cx - start_x)) >> bd ));
    }

    void finish() {
        if (debug && !check((uint8_t*)ptr())) {
            NRF51_CRASH("Out of bounds.");
        }
        if (dirty) *ptr() = chunk;
    }

    bool check(uint8_t* buf) {
        return buf < (buffer + (width * height) /8);
    }
};

//template<typename R, R dflt>
//class DisplayObjectVisitor {
//    virtual R visit(DisplayObject* o) { return dflt; }
//    virtual R visit(DisplayObjectContainer* o);
//    virtual R visit(Group* o);
//};

class DisplayObject { // pooled, 10 bytes.
  protected:

    coord_s_t _x;
    coord_s_t _y;
    pptr<DisplayObject> _parent;
    pptr<DisplayObject> _nextSibling;
  public:
    DisplayObject() : _x(0), _y(0), _parent(), _nextSibling() {}
    DisplayObject(DisplayObject* parent, coord_t x, coord_t y) :  _x(x), _y(y), _parent(parent), _nextSibling() {}
    // generated Copy Constructor is fine.

    virtual size_t getSize() const {
        return sizeof(DisplayObject);
    }

    void* operator new(size_t sz, Pool* p) {
        return p->allocate(sz);
    }

    virtual ~DisplayObject();

    Pool* getPool() {
        return pool_registry.get();
    }

    DisplayObject* getParent() {
        return _parent;
    }

    DisplayObject* getNextSibling() {
        return _nextSibling;
    }

    void setNextSibling(DisplayObject* d) {
        _nextSibling = d;
    }

    coord_t getX() {
        return (coord_t)_x;
    }

    DisplayObject& setX(coord_t x) {
        _x = x;
        return *this;
    }

    coord_t getY() {
        return (coord_t)_y;
    }

    DisplayObject& setY(coord_t y) {
        _y = y;
        return *this;
    }

    virtual void draw(RenderContext& ctx) = 0;
    virtual bool intersects(coord_t origin_x, coord_t origin_y,
            coord_t width, coord_t height) = 0;
    virtual bool intersects(RenderContext& ctx) {
        return intersects(ctx.start_x, ctx.start_y, ctx.width, ctx.height);
    }

    virtual void cleanup(RenderContext& ctx, void* mem){
    }
};



class DisplayObjectContainer : public DisplayObject { // 12 bytes.
    // TODO should this really inherit from DisplayObject?  Does it need x and y?  This would save us 4 bytes and we could maybe fit matrix in a 24 byte Group?
protected:

    pptr<DisplayObject> _firstChild;
    pptr<DisplayObject> _lastChild;

public:

    DisplayObjectContainer() : _firstChild(NULL), _lastChild(NULL) {}
    DisplayObjectContainer(DisplayObject* parent) : DisplayObject(parent, 0,0), _firstChild(NULL), _lastChild(NULL) {}
    // generated Copy Constructor is fine.

    void* operator new(size_t sz, Pool* p) {
        return p->allocate(sz);
    }

    virtual size_t getSize() const {
        return sizeof(DisplayObjectContainer);
    }

    ~DisplayObjectContainer();

    void clear() {
        DisplayObject* d = getFirstChild();
        while (d) {
            removeChild(d);
            d = getFirstChild();
        }
    }

    DisplayObject* getFirstChild() {
        return _firstChild;
    }

    DisplayObject* getLastChild() {
        return _lastChild;
    }

    DisplayObject* appendChild(DisplayObject * child) {
        if (!_firstChild) {
            _lastChild = _firstChild = child;
        } else {
            _lastChild->setNextSibling(child);
            _lastChild = child;
        }
        return child;
    }

    void removeChild(DisplayObject* child) {
        if (child->getParent() != this) {
            NRF51_CRASH("Child is not ours.");
        }
        DisplayObject* sib;
        for(sib = getFirstChild(); sib && sib->getNextSibling() != child; sib = sib->getNextSibling()) {
            /*nothing*/
        }
        if (sib) {
            sib->setNextSibling(child->getNextSibling());
        }
        if (child == getFirstChild()) {
            _firstChild = child->getNextSibling();
        }
        if (child == _lastChild) {
            _lastChild = sib;
        }

        size_t s = child->getSize();
        child->~DisplayObject();
        getPool()->release(s, child);
    }

    typedef function<bool(DisplayObject*)> find_first_t;
    DisplayObject* find_first(find_first_t f) {
        for(DisplayObject * d = _firstChild; d != NULL; d = d->getNextSibling()) {
            if (f(d)) return d;
            DisplayObjectContainer* dc = dynamic_cast<DisplayObjectContainer*>(d);
            if (dc) {
                DisplayObject* ret = dc->find_first(f);
                if (ret != NULL) return ret;
            }
        }
        return NULL;
    }

    template<typename R>
    R depth_first( function<R(DisplayObject*)> f, R dflt) {
        for(DisplayObject * d = _firstChild; d != NULL; d = d->getNextSibling()) {
            if (f(d)) return d;
        }
        return dflt;
    }

    /** Returns true if any part of this DisplayObject falls within the given view port. */
    virtual bool intersects(coord_t origin_x, coord_t origin_y,
                    coord_t width, coord_t height) {
        for(DisplayObject * child = getFirstChild(); child != NULL; child = child->getNextSibling()) {
            if (child->intersects(origin_x, origin_y, width, height)) return true;
        }
        return false;
    }

    /** Draw the section of the display that fits in the given view port.  Draw into the given buffer,
    * starting at start_offset, and skip stride after every row.
    */
    virtual void draw(RenderContext& ctx) {
        for(DisplayObject * child = getFirstChild(); child != NULL; child = child->getNextSibling()) {
            pool_registry.get()->check(child);
            child->draw(ctx);
        }
    }

    virtual void cleanup(RenderContext& ctx, void* mem) {

        for(DisplayObject * child = getFirstChild(); child != NULL; child = child->getNextSibling()) {
            child->cleanup(ctx, mem);
        }

    }

};

/** A DisplayObject with rectangular bounds: ie with a height and width. */
class RectDisplayObject : public DisplayObject { // 15 bytes
protected:
    bool  _fill;
    color_t _color;

public:
    RectDisplayObject(DisplayObject* parent, coord_t x, coord_t y,
             bool fill, color_t color) :
    DisplayObject(parent, x, y), _fill(fill), _color(color) {}

    void* operator new(size_t sz, Pool* p) {
        return p->allocate(sz);
    }
    // generated copy constructor is fine.
    virtual ~RectDisplayObject();

    virtual size_t getSize() const {
        return sizeof(RectDisplayObject);
    }
    virtual coord_t getWidth() = 0;
    virtual coord_t getHeight() = 0;
    bool getFill() {
        return _fill;
    }
    color_t getColor() {
        return _color;
    }


    virtual bool intersects(coord_t origin_x, coord_t origin_y,
            coord_t width, coord_t height) {
        return rectOverlap(rect(origin_x, origin_y, width, height), rect(getX(), getY(), getWidth(), getHeight()));
    }

protected:

    // after http://stackoverflow.com/a/306379/1409535



    bool rectOverlap(rect A, rect B)
    {
        bool xOverlap = valueInRange(A.x, B.x, B.x + B.width) ||
                valueInRange(B.x, A.x, A.x + A.width);

        bool yOverlap = valueInRange(A.y, B.y, B.y + B.height) ||
                valueInRange(B.y, A.y, A.y + B.height);

        return xOverlap && yOverlap;
    }

    bool valueInRange(int value, int min, int max)
    { return (value >= min) && (value <= max); }
};

class Line : public RectDisplayObject {
protected:
    coord_t x0;
    coord_t y0;
    coord_t dx;
    coord_t dy;
public:
    Line (DisplayObject* parent, coord_t startX, coord_t startY, coord_t endX, coord_t endY,bool fill,color_t color) :
        RectDisplayObject(parent,std::min(startX,endX),std::min(startY,endY),fill,color)
    {
        x0 = startX;
        y0 = startY;
        dx = endX - startX;
        dy = endY - startY;
    }

    virtual coord_t getWidth () { return std::abs(dx); }
    virtual coord_t getHeight () { return std::abs(dy); }

    virtual void draw(RenderContext& ctx);

    void* operator new(size_t sz, Pool* p) {
        return p->allocate(sz);
    }
    virtual ~Line();
};

class Circle : public RectDisplayObject {

};

class Path : public DisplayObject {  // Rect?

};

class Rectangle : public RectDisplayObject {
protected:
    coord_t _width;
    coord_t _height;

public:

    Rectangle (DisplayObject* parent, coord_t x, coord_t y, coord_t w, coord_t h, bool fill, color_t color) :
        RectDisplayObject(parent,x,y,fill,color),
        _width(w),
        _height(h) {}

    virtual void draw(RenderContext& ctx);


    virtual coord_t getWidth() { return _width; }
    virtual coord_t getHeight() { return _height; }

    void* operator new(size_t sz, Pool* p) {
        return p->allocate(sz);
    }
    virtual ~Rectangle();
};

struct PMatrix { // pooled, 18 bytes.
    coord_mat_3_s_t m; // does this really need to be a 3x3 matrix?  do we really want to support arbitrary rotation?
    PMatrix() : m(0) {}
    PMatrix(coord_mat_3_t mat) : m(mat) {}
    void* operator new(size_t sz, Pool* p) {
        return p->allocate(sz);
    }

};

class Group : public DisplayObjectContainer {

    pptr<PMatrix> _transform;
public:
    Group() : _transform(new (getPool()) PMatrix()) {}
    Group(const coord_mat_3_t& mat) : _transform(new (getPool()) PMatrix(mat)) {}
    virtual ~Group();

    void* operator new(size_t sz, Pool* p) {
        return p->allocate(sz);
    }

    virtual size_t getSize() const {
        return sizeof(Group);
    }

    void setTransform(const coord_mat_3_t& mat) {
        _transform->m = mat;
    }

    coord_mat_3_t getTransform() {
        return _transform->m;
    }

    // TOOD:
    //virtual void draw(RenderContext& ctx);
    //virtual bool intersects(coord_t origin_x, coord_t origin_y, coord_t width, coord_t height);
};

class Checkerboard : public RectDisplayObject {
protected:
    coord_t _width;
    coord_t _height;
public:
    Checkerboard(DisplayObject* parent, coord_t x, coord_t y, coord_t w, coord_t h,
            color_t color) : RectDisplayObject(parent, x, y, true, color),
        _width(w),
        _height(h)
    {}
    // generated destructor and copy constructor are fine.
    void* operator new(size_t sz, Pool* p) {
        return p->allocate(sz);
    }
    virtual ~Checkerboard();


    virtual coord_t getWidth() { return _width; }
    virtual coord_t getHeight() { return _height; }

    virtual void draw(RenderContext& ctx);
};



#endif /* _DisplayObject_h */