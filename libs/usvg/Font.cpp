//
// Created by Christopher Mason on 5/9/13.
// Copyright (c) 2013 Mason. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//


#include "Font.h"

#include "Debug.h"

Font::Font(const unsigned char* addr, const unsigned char* end) : debug(false) {
    info = (FontInfo*)addr;
    glyphs = (FontGlyph*) (((uint32_t*)&(info->offset_table)) + info->number_of_glyphs);
    this->end = (void*)end;

    int count = 0;
    uint32_t total_width = 0;
    uint16_t x_width = 0;
    for(int i = 1; i < info->number_of_glyphs; ++i) {
        uint16_t offset = info->offset_table[i].offset;

        if (offset) {
            if (((uint32_t*)glyphs + offset) > ((uint32_t*)end)) {
                Debug.print("Glyph ");
                Debug.print(i);
                Debug.print(" ('");
                Debug.print((char)info->offset_table[i].codepoint);
                Debug.print("') has invalid offset ");
                Debug.print(offset);
                Debug.println(".");
                Debug.flush();
                continue;
            }

            count++;
            total_width += glyphs[offset].horizontal_advance;
            if (info->offset_table[i].codepoint == 'X') {
                x_width = glyphs[offset].horizontal_advance;
            }
        }
    }


    advance = x_width ? x_width : total_width / count;

    if (debug) {
        Debug.print("Font has ");
        Debug.print(count);
        Debug.print(" characters, ");
        Debug.print(total_width);
        Debug.print(" total width, and ");
        Debug.print(advance);
        Debug.print(" advance");
        Debug.println(".");
        Debug.flush();
    }
}

FontGlyph* Font::getGlyph(wchar_t c) const {
    FontOffset* off = info->offset_table;
    FontGlyph* glyph = 0;
    int start = 0, end = info->number_of_glyphs;
    while(start <= end) {
        // 1 2 3 4 5 6
        // a b c d g h  look for f
        // start = 3 end = 5  end-start = 2.  2/2 = 1.  3 + 1 = 4
        // start = 4 end = 5 end - start = 1. 1/2 = 0. 4 + 0 = 4
        // start = 5 end = 5   pivot = 5
        int pivot = start + (end - start) / 2;
        uint16_t codepoint = off[pivot].codepoint;
//        if (debug) {
//            Debug.print("looking for ");
//            Debug.print((char)c);
//            Debug.print(" at ");
//            Debug.print(pivot);
//            Debug.print(" (");
//            Debug.print((char)codepoint);
//            Debug.print("), start = ");
//            Debug.print(start);
//            Debug.print(", end = ");
//            Debug.print(end);
//            Debug.println(".");
//            Debug.flush();
//        }
        if (codepoint == c) {
            uint16_t offset = off[pivot].offset;
            if(offset) {
                glyph = (FontGlyph*) (((uint32_t *) glyphs)+offset);
            }
            break;
        } else if (codepoint < c) {
            start = pivot + 1;
        } else {
            end = pivot - 1;
        }
    }

    if ((!glyph) && debug) {
        Debug.print("Couldn't find ");
        Debug.print((char)c);
        Debug.println();
        Debug.flush();
    }
    return glyph;
}

rect Font::measureChar(wchar_t c) const {
    FontGlyph* glyph = getGlyph(c);
    if (glyph) return measureGlyph(c, glyph);
    return rect(); // TODO: should this return a reasonable advance?
}
rect Font::measureGlyph(wchar_t c, FontGlyph* glyph) const {
    return rect(0,0,glyph->bitmap_width + glyph->offset_left,glyph->bitmap_height + glyph->offset_top);
}

rect Font::measureString(const string& s) const {
    rect ret;
    for(char c : s) {
        FontGlyph* glyph = getGlyph(c);
        rect cr = measureGlyph(c, glyph);
        if (cr.width < glyph->horizontal_advance) cr.width = glyph->horizontal_advance;
        ret.height = std::max(ret.height, cr.height);
        ret.width += cr.width;
    }
    return ret;
}

uint16_t Font::drawChar(RenderContext& ctx, coord_t startx, coord_t starty, wchar_t c, uint16_t foreground, uint32_t background) const {
    FontGlyph* glyph = getGlyph(c);
    if (glyph) return drawGlyph(ctx, startx, starty, c, glyph, foreground, background);
    return advance;
}

uint16_t Font::drawGlyph(RenderContext& ctx, coord_t startx, coord_t starty, wchar_t c, FontGlyph* glyph, uint16_t foreground, uint32_t background) const {

    coord_t w = ctx.width;
    coord_t h = ctx.height;


    if ((startx > ctx.start_x + w) || (starty > ctx.start_y + h)) {
        return 0;
    }

    coord_t glyphx = 0;
    coord_t glyphy = 0;
    coord_t x = startx + glyph->offset_left;
    coord_t y = starty + glyph->offset_top;
    coord_t maxx = x + glyph->bitmap_width;
    coord_t maxy = y + glyph->bitmap_height;
    if (x < 0) {
        glyphx += -x;
        x = 0;
    }
    if (y < 0) {
        glyphy += -y;
        y = 0;
    }
    if (maxy > ctx.start_y + h) maxy = ctx.start_y + h;
    if (maxx > ctx.start_x + w) maxx = ctx.start_x + w;

    coord_t outx = std::max((coord_t)x, ctx.start_x); // this is where we'll start drawing.
    coord_t outy = std::max((coord_t)y, ctx.start_y);
    coord_t outmaxx = std::min((coord_t)maxx, (coord_t)(ctx.start_x+ctx.width));  // this is where we'll stop drawing.
    coord_t outmaxy = std::min((coord_t)maxy, (coord_t)(ctx.start_y+ctx.height));
    glyphx += outx-x;
    glyphy += outy-y;
    if (glyphy < 0) {
        outy += -glyphy;
        glyphy = 0;
    }
    if (glyphx < 0) {
        outx += -glyphx;
        glyphx = 0;
    }


    /*

     screen: +----------------------------------------------+
             |                                              |
             |   ctx: +------------------+                  |
             |        |                  |                  |
             |        |     glyph: +-----+---+              |
             |        |           /|     |   |              |
             |        | outx, outy | .   |   |              |
             |        |            +-----+---+              |
             |        |                  |\                 |
             |        +------------------+ outmaxx, outmaxy |
             +----------------------------------------------+
                                     . = x,y

     */




    if (maxx > 0 && maxy > 0
            && outx < outmaxx && outy < outmaxy
            && (outx < ctx.start_x + w) && (outy < ctx.start_y + h)) {

        if (debug) {
            Debug.print((char)c);

            Debug.print(": ");
            Debug.print("ctx y=");
            Debug.print(ctx.start_y);
            Debug.print(", startx=");
            Debug.print(startx);
            Debug.print(", starty=");
            Debug.print(starty);
            Debug.print(", x=");
            Debug.print(x);
            Debug.print(", y=");
            Debug.print(y);
            Debug.print(", maxx=");
            Debug.print(maxx);
            Debug.print(", maxy=");
            Debug.print(maxy);
            Debug.print(", glyphx=");
            Debug.print(glyphx);
            Debug.print(", glyphy=");
            Debug.print(glyphy);
            Debug.println();
            Debug.flush();
        }

#ifdef USE_RENDER_CONTEXT

        drawPixels(ctx, glyph, outx, outy, glyphx, glyphy, outmaxx, outmaxy, foreground);

#else /* USE_RENDER_CONTEXT */
        uint8_t* buf = ctx.buffer + ((outx - ctx.start_x)/8)
                                  + ((outy - ctx.start_y) * ctx.width / 8)
                                  + ((outy - ctx.start_y) * ctx.stride);



        if (!ctx.check(buf)) {
            NRF51_CRASH("Render buffer exceeded before writing glyph.");
        }
        uint8_t bmask = (outx - ctx.start_x) % 8;

        for(; outy < outmaxy; ++outy, glyphy++) {
            uint8_t bword = *buf;
            if (debug) {
                Debug.print(outy);
                Debug.print(": ");
            }
            for(uint16_t xx = outx, gx=glyphx; xx < outmaxx; ++xx, gx++) {
                uint16_t n = glyph->bitmap_width * glyphy + gx;
                uint16_t gword = glyph->bitmap[n / 16];
                uint16_t gmask = 1 << (n % 16);

                bool on = gword & gmask;
                if (debug) {
                    Debug.print(xx);
                    Debug.print("=");
                    Debug.print(on);
                    Debug.print(" ");
                }
                if(on) {

                    if (foreground) {
                        bword |= 1 << bmask;
                    } else {
                        bword &= ~(1 << bmask);
                    }
                }
                bmask++;
                if (bmask == 8) {
                    *buf = bword;
                    if (xx < outmaxx-1) {
                        buf++;
                        bword = *buf;
                        bmask = 0;

                        if (!ctx.check(buf)) {
                            NRF51_CRASH("Render buffer exceeded while writing glyph line.");
                        }
                    }

                }
            }
            if (debug) {
                Debug.println();
            }
            *buf= bword;
            if (outy != outmaxy -1) {
                buf += (outmaxx - ctx.width)/8;
                buf += ctx.stride;
                // TODO: mask stride?
                buf += (outx - ctx.start_x) / 8;
                bmask = (outx - ctx.start_x) % 8;

                if (!ctx.check(buf)) {
                    NRF51_CRASH("Render buffer exceeded after writing glyph line");
                }
            }

        }
#endif /* USE_RENDER_CONTEXT */
    }
//    if (debug) {
//        Debug.println();
//        Debug.print("advance =");
//        Debug.println(glyph->horizontal_advance);
//        Debug.flush();
//    }


    return glyph->horizontal_advance;

}

void Font::drawPixels(RenderContext& ctx, FontGlyph* glyph, coord_t outx, coord_t outy, coord_t glyphx, coord_t glyphy, coord_t outmaxx, coord_t outmaxy, color_t color) const {
    for(; outy < outmaxy; ++outy, glyphy++) {
        for(uint16_t xx = outx, gx=glyphx; xx < outmaxx; ++xx, gx++) {
            uint16_t n = glyph->bitmap_width * glyphy + gx;
            uint16_t gword = glyph->bitmap[n / 16];
            uint16_t gmask = 1 << (n % 16);

            bool on = gword & gmask;
            if (on) ctx.setPixel(xx, outy, color);
        }
    }

}


void Text::draw(RenderContext& buf) {
    const char* str = _text->cstr ? _text->cstr : _text->str.c_str();
    coord_t x = getX();
    for(int i = 0; str[i] != 0; ++i) {
        x += _font->drawChar(buf, x, getY(), str[i], _color, 0);
    }
}

bool Text::intersects(coord_t origin_x, coord_t origin_y, coord_t width, coord_t height) {
    return true; // TODO
}


//
//void Display::drawString(uint16_t x, uint16_t y, const char* str, const Font& font) {
//    const Font* oldFont = getFont();
//    setFont(&font);
//    drawString(x,y,str);
//    setFont(oldFont);
//}
//
//void Display::drawString(uint16_t x, uint16_t y, const char* str) {
//
//    if (!font) return;
//
//    for(int i = 0; str[i] != 0; ++i) {
//        x += font->drawChar(_gfx, x, y, str[i], 1, 0);
//    }
//
//}
//
//void Display::drawString(uint16_t x, uint16_t y, const string& str, const Font& font) {
//    const Font* oldFont = getFont();
//    setFont(&font);
//    drawString(x,y,str);
//    setFont(oldFont);
//}
//
//void Display::drawString(uint16_t x, uint16_t y, const string& str) {
//    if (!font) return;
//
//    for(wchar_t c : str) {
//        x += font->drawChar(_gfx, x, y, c, 1, 0);
//    }
//
//}

