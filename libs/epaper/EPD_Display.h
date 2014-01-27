#ifndef _EPD_Display_h
#define _EPD_Display_h

#include "EPD.h"
#include "Display.h"

#include <stdint.h>

void setupSPI(int Pin_MISO, int Pin_MOSI, int Pin_Sclk);

class EPD_Display : public Display {


    DisplayContext _ctx;
    EPD_Class& _epd;

    // Ok, so the 2.7" display has too many pixels (5808 bytes worth) to fit in memory once, let alone twice,
    // as the repaper code would have us do.  So, we use a display list structure and render it on the fly in tiles.
    // we keep two display lists, one for the previous image and one for the current one.  The primary element
    // on the display list is currently a Run-Length-Encoded image (RLEImage) sent from the host.  This could
    // change in the future as we overlay additional text or graphics.

    DisplayList _ping;
    DisplayList _pong;
    DisplayList* _dl; // the display list we're currently drawing into (the one we will display next, not the one currently displayed).

    uint32_t _updates;

  public:

    EPD_Display(Pool* p, EPD_Class& epaper)
        : _ctx(epaper.width(), epaper.height()), _epd(epaper), _ping(p, _ctx), _pong(p, _ctx), _dl(&_ping), _updates(0) {
    }
    virtual ~EPD_Display();

    virtual int_coord_t height() {
        return _ctx.height;
    }

    virtual int_coord_t width() {
        return _ctx.width;
    }


    /** Returns the display list we're currently drawing into (the one we will display next, not the one currently displayed). */
    DisplayList& active() {
        return *_dl;
    }
    /** Returns the display list currently displayed. */
    DisplayList& last() {
        return _dl == &_ping ? _pong : _ping;
    }

    /** Show the current display list on the screen. */
    virtual EPD_Display& display();

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TODO replace this section with ADisplay...

    EPD_Display& clear() {
        active().clear();
        return *this;
    }

    virtual void invertDisplay(bool i) {
        active().invertDisplay(i);
    }

    virtual EPD_Display& drawString(coord_t x, coord_t y, const string& str, color_t color) {
        active().drawString(x,y , str, color);
        return *this;
    }

    virtual EPD_Display& drawString(coord_t x, coord_t y, const string& str, Font& font, color_t color) {
        active().drawString(x,y, str, font, color);
        return *this;
    }

    virtual EPD_Display& drawString(coord_t x, coord_t y, const char* str, Font& font, color_t color) {
        active().drawString(x,y, str, font, color);
        return *this;
    }

    virtual EPD_Display& setFont(Font* f) {
        active().setFont(f);
        return *this;
    }

    virtual EPD_Display& appendChild(DisplayObject *obj) {
        active().appendChild(obj);
        return *this;
    }

    virtual RLEImage& drawImage(const RLEImageMessage& message) {
        return active().drawImage(message);
    }

    virtual RLEImage& drawImage() {
        return active().drawImage();
    }

    virtual EPD_Display& drawPath(string string1) {
        active().drawPath(string1);
        return *this;
    }

    virtual EPD_Display& drawLine(coord_t x0, coord_t y0, coord_t x1, coord_t y1, color_t color){
        active().drawLine(x0, y0, x1, y1, color);
        return *this;
    }

    virtual EPD_Display& drawFastVLine(coord_t x, coord_t y, coord_t h, color_t color) {
        active().drawFastVLine(x, y, h, color);
        return *this;
    }

    virtual EPD_Display& drawFastHLine(coord_t x, coord_t y, coord_t w, color_t color){
        active().drawFastHLine(x, y, w, color);
        return *this;
    }

    virtual EPD_Display& drawRect(coord_t x, coord_t y, coord_t w, coord_t h, color_t color) {
        active().drawRect(x, y, w, h, color);
        return *this;
    }

    virtual EPD_Display& fillRect(coord_t x, coord_t y, coord_t w, coord_t h, color_t color) {
        active().fillRect(x, y, w, h, color);
        return *this;
    }

    virtual EPD_Display& fillCheckerboard(coord_t x, coord_t y, coord_t w, coord_t h,
            color_t color) {
        active().fillCheckerboard(x, y, w, h, color);
        return *this;
    }

    virtual EPD_Display& fillScreen(color_t color) {
        active().fillScreen(color);
        return *this;
    }

    virtual EPD_Display& drawCircle(coord_t x0, coord_t y0, coord_t EPD_Display, color_t color) {
        active().drawCircle(x0, y0, EPD_Display, color);
        return *this;
    }

    virtual EPD_Display& fillCircle(coord_t x0, coord_t y0, coord_t EPD_Display, color_t color){
        active().fillCircle(x0, y0, EPD_Display, color);
        return *this;
    }

    virtual EPD_Display& drawTriangle(coord_t x0, coord_t y0, coord_t x1, coord_t y1, coord_t x2, coord_t y2, color_t color) {
        active().drawTriangle(x0, y0, x1, y1, x2, y2, color);
        return *this;
    }

    virtual EPD_Display& fillTriangle(coord_t x0, coord_t y0, coord_t x1, coord_t y1, coord_t x2, coord_t y2, color_t color) {
        active().fillTriangle(x0, y0, x1, y1, x2, y2, color);
        return *this;
    }

    virtual EPD_Display& drawRoundRect(coord_t x0, coord_t y0, coord_t w, coord_t h, coord_t radius, color_t color) {
        active().drawRoundRect(x0, y0, w, h, radius, color);
        return *this;
    }

    virtual EPD_Display& fillRoundRect(coord_t x0, coord_t y0, coord_t w, coord_t h, coord_t radius, color_t color) {
        active().fillRoundRect(x0, y0, w, h, radius, color);
        return *this;
    }

    virtual EPD_Display& drawBitmap(coord_t x, coord_t y, const uint8_t *bitmap, coord_t w, coord_t h, color_t color) {
        active().drawBitmap(x, y, bitmap, w, h, color);
        return *this;
    }

    virtual EPD_Display& drawChar(coord_t x, coord_t y, unsigned char c, color_t color, color_t bg, uint8_t size) {
        active().drawChar(x, y, c, color, bg, size);
        return *this;
    }

    virtual EPD_Display& setRotation(coord_t EPD_Display) {
        active().setRotation(EPD_Display);
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
//    virtual EPD_Display& setCursor(coord_t x, coord_t y) {
//    }
//
//    virtual EPD_Display& setTextColor(color_t c);
//
//    virtual EPD_Display& setTextColor(color_t c, color_t bg);
//
//    virtual EPD_Display& setTextSize(uint8_t s);
//
//    virtual EPD_Display& setTextWrap(bool w);

    virtual Group& pushGroup(const coord_mat_3_t& transform){
        return active().pushGroup(transform);
    }
    virtual Group& popGroup() {
        return active().popGroup();
    }
    virtual Group& peekGroup() {
        return active().peekGroup();
    }

    // END REPLACE
    ////////////////////////////////////////////////////////////////////////////////////////////////////


protected:

    void draw(DisplayList& dl, EPD_stage stage);

};

#endif /* _EPD_Display_h */
