
#ifndef __Display_h
#define __Display_h

#include "DisplayObject.h"
#include "RLEImage.h"
#include "Font.h"

/** Implementations of ABC Display wrap a DisplayList and provide the ability to draw it on some device. */
class Display /* TODO : public Print */ { // Protocol
public:

    virtual ~Display();

    virtual size_t getSize() {
        return sizeof(Display);
    }

    // These signatures come from the Adafruit GFX library that many open source Arduino projects use.
    // Color here is pretty irrelevant, but I've left it in place for signature compatibility

    // not implemented:
    // void drawPixel(int16_t x, int16_t y, uint16_t color)=0;


    virtual int_coord_t height() = 0;
    virtual int_coord_t width() = 0;

    // can we realistically do this?
    virtual void invertDisplay(bool i) = 0;


    virtual Display& drawString(coord_t x, coord_t y, const string& str, color_t color) = 0;  // TODO: utf8 Pooled string
    virtual Display& drawString(coord_t x, coord_t y, const string& str, Font& font, color_t color) = 0;
    virtual Display& drawPath(string) = 0;

    virtual Display& drawLine(coord_t x0, coord_t y0, coord_t x1, coord_t y1,
            color_t color) = 0;
    virtual Display& drawFastVLine(coord_t x, coord_t y, coord_t h, color_t color) = 0;
    virtual Display& drawFastHLine(coord_t x, coord_t y, coord_t w, color_t color) = 0;
    virtual Display& drawRect(coord_t x, coord_t y, coord_t w, coord_t h,
            color_t color) = 0;
    virtual Display& fillRect(coord_t x, coord_t y, coord_t w, coord_t h,
            color_t color) = 0;
    virtual Display& fillScreen(color_t color) = 0;
    virtual Display& fillCheckerboard(coord_t x, coord_t y, coord_t w, coord_t h,
            color_t color) = 0;

    virtual Display& drawCircle(coord_t x0, coord_t y0, coord_t r, color_t color) = 0;
    virtual Display& fillCircle(coord_t x0, coord_t y0, coord_t r, color_t color) = 0;

    virtual Display& drawTriangle(coord_t x0, coord_t y0, coord_t x1, coord_t y1,
            coord_t x2, coord_t y2, color_t color) = 0;
    virtual Display& fillTriangle(coord_t x0, coord_t y0, coord_t x1, coord_t y1,
            coord_t x2, coord_t y2, color_t color) = 0;
    virtual Display& drawRoundRect(coord_t x0, coord_t y0, coord_t w, coord_t h,
            coord_t radius, color_t color) = 0;
    virtual Display& fillRoundRect(coord_t x0, coord_t y0, coord_t w, coord_t h,
            coord_t radius, color_t color) = 0;

    virtual Display& drawBitmap(coord_t x, coord_t y,
            const uint8_t *bitmap, coord_t w, coord_t h,
            color_t color) = 0;

    virtual Display& drawChar(coord_t x, coord_t y, unsigned char c,
            color_t color, color_t bg, uint8_t size) = 0;


    // are these realistic?  we indeed need to handle rotation but I think it ends up being so different
    // that we have to send it back to the host for a totally different lyaout.
    virtual Display& setRotation(coord_t r) = 0;
    virtual uint8_t getRotation(void) = 0;

    // Methods associated with Printable, so that Display can be used as a text output device.  should we support these? they're a little weird.
//    virtual size_t write(uint8_t) = 0;
//    virtual Display& setCursor(coord_t x, coord_t y) = 0;  // what should this even do?
//    virtual Display& setTextColor(color_t c) = 0;
//    virtual Display& setTextColor(color_t c, color_t bg) = 0;
//    virtual Display& setTextSize(uint8_t s) = 0;  // what should this even do?
//    virtual Display& setTextWrap(bool w) = 0;

    // These are additional methods not in the original Adafruit interface.

    /** Pushes a matrix transform.  This transform persists until a call to popGroup(). */
    virtual Group& pushGroup(const coord_mat_3_t& transform) = 0;

    /** Removes the bottom most group from the Group stack but leaves it in the display list. */
    virtual Group& popGroup() = 0;

    /** Returns but does not remove the bottom most Group. */
    virtual Group& peekGroup() = 0;

    /** Set the font for use with the write() methods or when no font is specified to drawString().
    * TODO can we make this take something other than a pointer?
    * */
    virtual Display& setFont(Font* f) = 0;

    /** Appends the given child to the most recently pushed Group, or if none, to the root.
    * This takes ownership of the given DisplayObject.
     * TODO this is one of the only methods that takes a pointer.  Should we make this protected?
      * */
    virtual Display& appendChild(DisplayObject* obj) = 0;

    /** Appends the given run length encoded image data to the first image found under the current group.
    * Returns the image.  */
    virtual RLEImage& drawImage(const RLEImageMessage& message) = 0;
    virtual RLEImage& drawImage() = 0;

    /** Removes everything from this Display, including all Group transforms. Does not reset any state like Font or height/width. */
    virtual Display& clear() = 0;

    /** Renders the current content of the Display to the physical device. */
    virtual Display& display() = 0;

};


class DisplayContext {

public:
    int_coord_t width, height;
    Font*          font;

    // eventually: glyph cache, etc.

    DisplayContext(coord_t w, coord_t h) :  width(w), height(h) {}
};


/** ABC Display that forwards all drawing ops to an "active" DisplayList.
* This turns out not to work.  See http://stackoverflow.com/questions/18678915/c-abstract-base-template-for-forwarding-with-covariant-return
* */
template<typename R>
class ADisplay : public Display {
  protected:
    DisplayContext _ctx;

  public:

    ADisplay(coord_t width, coord_t height) : _ctx(width, height) {}

    /** Returns the currently "active" DisplayList that should be used for drawing operations that will be shown
    * on the next call to display(). */
    virtual DisplayList& active() = 0;

    virtual int_coord_t height() {
        return _ctx.height;
    }

    virtual int_coord_t width() {
        return _ctx.width;
    }

    virtual R& display() = 0;

    R& clear() {
        active().clear();
        return *this;
    }

    virtual void invertDisplay(bool i) {
        active().invertDisplay(i);
    }

    virtual R& drawString(coord_t x, coord_t y, const string& str, color_t color) {
        active().drawString(x,y , str, color);
        return *this;
    }

    virtual R& drawString(coord_t x, coord_t y, const string& str, Font& font, color_t color) {
        active().drawString(x,y, str, font, color);
        return *this;
    }

    virtual R& setFont(Font* f) {
        active().setFont(f);
        return *this;
    }

    virtual R& appendChild(DisplayObject *obj) {
        active().appendChild(obj);
        return *this;
    }

    virtual RLEImage& drawImage(const RLEImageMessage& message) {
        return active().drawImage(message);
    }

    virtual RLEImage& drawImage() {
        return active().drawImage();
    }

    virtual R& drawPath(string string1) {
        active().drawPath(string1);
        return *this;
    }

    virtual R& drawLine(coord_t x0, coord_t y0, coord_t x1, coord_t y1, color_t color){
        active().drawLine(x0, y0, x1, y1, color);
        return *this;
    }

    virtual R& drawFastVLine(coord_t x, coord_t y, coord_t h, color_t color) {
        active().drawFastVLine(x, y, h, color);
        return *this;
    }

    virtual R& drawFastHLine(coord_t x, coord_t y, coord_t w, color_t color){
        active().drawFastHLine(x, y, w, color);
        return *this;
    }

    virtual R& drawRect(coord_t x, coord_t y, coord_t w, coord_t h, color_t color) {
        active().drawRect(x, y, w, h, color);
        return *this;
    }

    virtual R& fillRect(coord_t x, coord_t y, coord_t w, coord_t h, color_t color) {
        active().fillRect(x, y, w, h, color);
        return *this;
    }

    virtual R& fillCheckerboard(coord_t x, coord_t y, coord_t w, coord_t h,
            color_t color) {
        active().fillCheckerboard(x, y, w, h, color);
        return *this;
    }

    virtual R& fillScreen(color_t color) {
        active().fillScreen(color);
        return *this;
    }

    virtual R& drawCircle(coord_t x0, coord_t y0, coord_t r, color_t color) {
        active().drawCircle(x0, y0, r, color);
        return *this;
    }

    virtual R& fillCircle(coord_t x0, coord_t y0, coord_t r, color_t color){
        active().fillCircle(x0, y0, r, color);
        return *this;
    }

    virtual R& drawTriangle(coord_t x0, coord_t y0, coord_t x1, coord_t y1, coord_t x2, coord_t y2, color_t color) {
        active().drawTriangle(x0, y0, x1, y1, x2, y2, color);
        return *this;
    }

    virtual R& fillTriangle(coord_t x0, coord_t y0, coord_t x1, coord_t y1, coord_t x2, coord_t y2, color_t color) {
        active().fillTriangle(x0, y0, x1, y1, x2, y2, color);
        return *this;
    }

    virtual R& drawRoundRect(coord_t x0, coord_t y0, coord_t w, coord_t h, coord_t radius, color_t color) {
        active().drawRoundRect(x0, y0, w, h, radius, color);
        return *this;
    }

    virtual R& fillRoundRect(coord_t x0, coord_t y0, coord_t w, coord_t h, coord_t radius, color_t color) {
        active().fillRoundRect(x0, y0, w, h, radius, color);
        return *this;
    }

    virtual R& drawBitmap(coord_t x, coord_t y, const uint8_t *bitmap, coord_t w, coord_t h, color_t color) {
        active().drawBitmap(x, y, bitmap, w, h, color);
        return *this;
    }

    virtual R& drawChar(coord_t x, coord_t y, unsigned char c, color_t color, color_t bg, uint8_t size) {
        active().drawChar(x, y, c, color, bg, size);
        return *this;
    }

    virtual R& setRotation(coord_t r) {
        active().setRotation(r);
        return *this;
    }

    virtual uint8_t getRotation() {
        return active().getRotation();
    }
//
//    virtual size_t write(uint8_t t) {
//        return active().write(t);
//    }
//
//    virtual R& setCursor(coord_t x, coord_t y) {
//    }
//
//    virtual R& setTextColor(color_t c);
//
//    virtual R& setTextColor(color_t c, color_t bg);
//
//    virtual R& setTextSize(uint8_t s);
//
//    virtual R& setTextWrap(bool w);

    virtual Group& pushGroup(const coord_mat_3_t& transform){
        return active().pushGroup(transform);
    }
    virtual Group& popGroup() {
        return active().popGroup();
    }
    virtual Group& peekGroup() {
        return active().peekGroup();
    }
};


class DisplayList : public Display {  // non-pooled
    Pool*                     _pool;
    DisplayContext&           _context;
    Group*                    _root;
    Group*                    _leaf;
  protected:
    void init() {
        _leaf = _root = new (_pool) Group();

    }

public:
    DisplayList(Pool* pool, DisplayContext& ctx) : _pool(pool), _context(ctx)  {
        init();
    }
    virtual ~DisplayList();

    DisplayList& display() {
        // this method doesn't really make sense in DisplayList...
        return *this;
    }

    Pool* getPool() {
        return _pool;
    }

    Group& pushGroup(const coord_mat_3_t& transform) {
        Group* g = new (_pool) Group(transform);
        _leaf->appendChild(g);
        _leaf = g;
        return *g;
    }

    Group& popGroup() {
        Group* g = _leaf;
        _leaf = dynamic_cast<Group*>(_leaf->getParent());
        return *g;
    }

    Group& peekGroup() {
        Group* g = dynamic_cast<Group*>(_leaf);
        return *g;
    }

    DisplayList& appendChild(DisplayObject* obj) { // takes ownership.  this is the only method that takes a pointer?
        peekGroup().appendChild(obj);
        return *this;
    }

//    template<typename R>
//    std::vector<R> map_depth_first(function<R(DisplayObject*)> f) {
//        std::vector<R> ret;
//
//        return ret;
//    }

    template<typename R>
    R depth_first( function<R(DisplayObject*)> f, R dflt) {
        return _root->depth_first(f, dflt);
    }

    DisplayObject* find_first(function<bool(DisplayObject*)>f) {
        return _root->find_first(f);
    }

    DisplayObject* root() {  // should this return a reference?
        return _root;
    }

    void draw(RenderContext& buf) {
        _root->draw(buf);
    }

    int_coord_t width() {
        return _context.width;
    }

    int_coord_t height() {
        return _context.height;
    }

    DisplayList& clear() {
        cleanup();
        init();
        return *this;
    }

    virtual RLEImage& drawImage() {
        RLEImage* ri = dynamic_cast<RLEImage*>(peekGroup().find_first([](DisplayObject* d) {
            return dynamic_cast<RLEImage*>(d) != 0;
        }));
        if (!ri) {
            ri = new (_pool) RLEImage(&peekGroup(), 0, 0, width(), height());
            appendChild(ri);
        }
        return *ri;
    }

    virtual RLEImage& drawImage(const RLEImageMessage& message) {
        RLEImage& ri = drawImage();
        ri.put(message);
        return ri;
    }

    virtual void invertDisplay(bool i) {
        // TODO can this be done?
    }

    virtual DisplayList& drawString(coord_t x, coord_t y, const string& str, color_t color) {
        return appendChild(new (_pool) Text(&peekGroup(), x, y, str, getFont(), color));
    }

    virtual DisplayList& drawString(coord_t x, coord_t y, const string& str, Font& font, color_t color) {
        return appendChild(new (_pool) Text(&peekGroup(), x, y, str, &font, color));
    }

    virtual DisplayList& drawString(coord_t x, coord_t y, const char* str, Font& font, color_t color) {
        return appendChild(new (_pool) Text(&peekGroup(), x, y, str, &font, color));
    }

    virtual DisplayList& drawPath(string string1) {
        // TODO
        return *this;
    }

    virtual DisplayList& drawLine(coord_t x0, coord_t y0, coord_t x1, coord_t y1, color_t color) {
        return appendChild(new (_pool) Line(&peekGroup(),x0,y0,x1,y1,0,color));
    }

    virtual DisplayList& drawFastVLine(coord_t x, coord_t y, coord_t h, color_t color) {
        // TODO
        return *this;
    }

    virtual DisplayList& drawFastHLine(coord_t x, coord_t y, coord_t w, color_t color) {
        // TODO
        return *this;
    }

    virtual DisplayList& drawRect(coord_t x, coord_t y, coord_t w, coord_t h, color_t color) {
        return appendChild(new (_pool) Rectangle(&peekGroup(),x,y,w,h,0,color));
    }

    virtual DisplayList& fillRect(coord_t x, coord_t y, coord_t w, coord_t h, color_t color) {
        return appendChild(new (_pool) Rectangle(&peekGroup(),x,y,w,h,1,color));
    }

    virtual DisplayList& fillCheckerboard(coord_t x, coord_t y, coord_t w, coord_t h, color_t color) {
        return appendChild(new (_pool) Checkerboard(&peekGroup(), x, y, w, h, color));
    }

    virtual DisplayList& fillScreen(color_t color) {
        // TODO
        return *this;
    }

    virtual DisplayList& drawCircle(coord_t x0, coord_t y0, coord_t r, color_t color) {
        // TODO
        return *this;
    }

    virtual DisplayList& fillCircle(coord_t x0, coord_t y0, coord_t r, color_t color) {
        // TODO
        return *this;
    }

    virtual DisplayList& drawTriangle(coord_t x0, coord_t y0, coord_t x1, coord_t y1, coord_t x2, coord_t y2, color_t color) {
        // TODO
        return *this;
    }

    virtual DisplayList& fillTriangle(coord_t x0, coord_t y0, coord_t x1, coord_t y1, coord_t x2, coord_t y2, color_t color) {
        // TODO
        return *this;
    }
    virtual DisplayList& drawRoundRect(coord_t x0, coord_t y0, coord_t w, coord_t h, coord_t radius, color_t color) {
        // TODO
        return *this;
    }

    virtual DisplayList& fillRoundRect(coord_t x0, coord_t y0, coord_t w, coord_t h, coord_t radius, color_t color) {
        // TODO
        return *this;
    }

    virtual DisplayList& drawBitmap(coord_t x, coord_t y, const uint8_t *bitmap, coord_t w, coord_t h, color_t color) {
        // TODO
        return *this;
    }

    virtual DisplayList& drawChar(coord_t x, coord_t y, unsigned char c, color_t color, color_t bg, uint8_t size) {
        // TODO
        return *this;
    }

    virtual DisplayList& setRotation(coord_t r) {
        // TODO
        return *this;
    }

    virtual uint8_t getRotation() {
        // TODO
        return 0;
    }

//    virtual size_t write(uint8_t t);
//
//    virtual Display& setCursor(coord_t x, coord_t y);
//
//    virtual Display& setTextColor(color_t c);
//
//    virtual Display& setTextColor(color_t c, color_t bg);
//
//    virtual Display& setTextSize(uint8_t s);
//
//    virtual Display& setTextWrap(bool w);

    virtual DisplayList& setFont(Font* f) {
        _context.font = f;
        return *this;
    }

    virtual Font* getFont() {
        return _context.font;
    }

  protected:
     void cleanup() {
         size_t s = _root->getSize();
         _root->~Group();
         _pool->release(s, _root);
     }

};



#endif /* __Display_h */
