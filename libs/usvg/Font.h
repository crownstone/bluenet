//
// Created by Christopher Mason on 5/9/13.
// Copyright (c) 2013 Mason. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//



#ifndef __Fonts_H_
#define __Fonts_H_

#include "DisplayObject.h"


/*
        borrowed from Pebble SDK

      FontInfo
          (uint8_t)  version
          (uint8_t)  max_height
          (uint16_t) number_of_glyphs
          (uint32_t) offset_table[]
          this table contains offsets into the glyph_table for characters 0x20 to 0xff
          each offset is counted in 32-bit blocks from the start of the glyph
          table. 16-bit offsets are keyed by 16-bit codepoints.
          packed:     (uint_16) codepoint
                      (uint_16) offset

      (uint32_t) glyph_table[]
          [0]: the 32-bit block for offset 0 is used to indicate that a glyph is not supported
          then for each glyph:
          [offset + 0]  packed:   (int_8) offset_top
                                  (int_8) offset_left,
                                 (uint_8) bitmap_height,
                                 (uint_8) bitmap_width (LSB)

          [offset + 1]           (int_8) horizontal_advance
                                 (24 bits) zero padding
            [offset + 2] bitmap data (unaligned rows of bits), padded with 0's at
            the end to make the bitmap data as a whole use multiples of 32-bit
            blocks
     */

struct __attribute__ ((__packed__)) __attribute__ ((aligned (1))) FontOffset {
    uint16_t codepoint;
    uint16_t offset;


};

struct __attribute__ ((__packed__)) __attribute__ ((aligned (1))) FontInfo {
    uint8_t version;
    uint8_t max_height;
    uint16_t number_of_glyphs;
    uint16_t wildcard_codepoint;
    FontOffset offset_table[1]; //[]
};

struct __attribute__ ((__packed__)) __attribute__ ((aligned (1))) FontGlyph {

    uint8_t bitmap_width;
    uint8_t bitmap_height;
    int8_t offset_left;
    int8_t offset_top;

    uint8_t reserved[3];
    int8_t horizontal_advance;
    uint16_t bitmap[1];
};

class Font {
  private:
    FontInfo* info;
    FontGlyph* glyphs;
    void* end;
    int16_t advance;

    bool debug;


  public:

    Font(const unsigned char* addr, const unsigned char* end);

    void setDebug(bool dbg) {
        debug = dbg;
    }

//drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size
    uint16_t drawChar(RenderContext& ctx, coord_t startx, coord_t starty, wchar_t c, uint16_t foreground, uint32_t background) const;
    FontGlyph* getGlyph(wchar_t c) const;
    uint16_t drawGlyph(RenderContext& ctx, coord_t startx, coord_t starty, wchar_t c, FontGlyph* glyph, uint16_t foreground, uint32_t background) const;

    rect measureChar(wchar_t c) const;
    rect measureGlyph(wchar_t c, FontGlyph* glyph) const;
    rect measureString(const string& s) const;

  protected:

    void drawPixels(RenderContext& ctx, FontGlyph* glyph,
                    coord_t outx, coord_t outy,
                    coord_t glyphx, coord_t glyphy,
                    coord_t outmaxx, coord_t outmaxy,
                    color_t color) const;

};


struct PString { // pooled, 12 bytes.
    // TODO this should get replaced with a for-realzies UTF-8, pooled string/rope implementation.
    const char * cstr;
    string str; // gcc string is at least 12 bytes.

    PString(const string& s) : cstr(0), str(s) {}
    PString(const char* s) : cstr(s) {}

    void* operator new(size_t sz, Pool* p) {
        return p->allocate(sz);
    }

};

class Text : public DisplayObject { // pooled, 19 bytes.

    Font*  _font;
    pptr<PString> _text;
    color_t _color;

public:
    Text(DisplayObject* parent, coord_t x, coord_t y, const string& text, Font* font, color_t color)
    : DisplayObject(parent, x, y), _font(font) , _text(new (getPool()) PString(text)), _color(color) {}
    Text(DisplayObject* parent, coord_t x, coord_t y, const char* text, Font* font, color_t color)
    : DisplayObject(parent, x, y), _font(font) , _text(new (getPool()) PString(text)), _color(color) {}
    void* operator new(size_t sz, Pool* p) {
        return p->allocate(sz);
    }
    virtual size_t getSize() const {
        return sizeof(Text);
    }
    ~Text() {
        _text->~PString();
        getPool()->release(sizeof(PString), _text);
    }
    virtual void draw(RenderContext& buf);

    virtual bool intersects(coord_t origin_x, coord_t origin_y, coord_t width, coord_t height);

};



#endif //__Fonts_H_
