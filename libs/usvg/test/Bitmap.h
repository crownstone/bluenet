
#ifndef _Bitmap_h
#define _Bitmap_h

#include <png.h>
#include "Display.h"


void abort_(const char * s, ...)
{
    va_list args;
    va_start(args, s);
    vfprintf(stderr, s, args);
    fprintf(stderr, "\n");
    va_end(args);
    abort();
}

class Bitmap : public Display {
    png_bytep * _row_pointers;

    DisplayContext _ctx;
    DisplayList _dl;


public:
    Bitmap(uint16_t w, uint16_t h) : _ctx(w, h), _dl(pool_registry.get(), _ctx) {
        _row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height());
        int rowbytes = ( width()/8 + 3) & ~0x03; // round up to multiple of 4.   http://stackoverflow.com/questions/2022179/c-quick-calculation-of-next-multiple-of-4
        int y;
        for (y=0; y<height(); y++) {
            _row_pointers[y] = (png_byte*) malloc(rowbytes);
            for(int i = 0; i < rowbytes; ++i) _row_pointers[y][i] = 0xFF;
        }
    }

    ~Bitmap() {

        /* cleanup heap allocation */
        for (int y=0; y<height(); y++)
            free(_row_pointers[y]);
        free(_row_pointers);
    }


    /** Returns the currently "active" DisplayList that should be used for drawing operations that will be shown
    * on the next call to display(). */
    DisplayList& active() {
        return _dl;
    }

    virtual int_coord_t height() {
        return _ctx.height;
    }

    virtual int_coord_t width() {
        return _ctx.width;
    }

    void drawPixel(int16_t x, int16_t y, uint16_t color) {
        if (!color) {
            _row_pointers[y][x/8] |= 1 << (8-x%8-1);
        } else {
            _row_pointers[y][x/8] &= ~(1 << (8-x%8-1));
        }
    }

    void write_png_file(char* file_name)
    {
        display();

        png_structp png_ptr;
        png_infop info_ptr;

        /* create file */
        FILE *fp = fopen(file_name, "wb");
        if (!fp)
            abort_("[write_png_file] File %s could not be opened for writing", file_name);


        /* initialize stuff */
        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (!png_ptr)
            abort_("[write_png_file] png_create_write_struct failed");

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
            abort_("[write_png_file] png_create_info_struct failed");

        if (setjmp(png_jmpbuf(png_ptr)))
            abort_("[write_png_file] Error during init_io");

        png_init_io(png_ptr, fp);


        /* write header */
        if (setjmp(png_jmpbuf(png_ptr)))
            abort_("[write_png_file] Error during writing header");

        png_set_IHDR(png_ptr, info_ptr, width(), height(),
                1, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

        png_set_packing(png_ptr);

        png_write_info(png_ptr, info_ptr);


        /* write bytes */
        if (setjmp(png_jmpbuf(png_ptr)))
            abort_("[write_png_file] Error during writing bytes");

        png_write_image(png_ptr, _row_pointers);


        /* end write */
        if (setjmp(png_jmpbuf(png_ptr)))
            abort_("[write_png_file] Error during end of write");

        png_write_end(png_ptr, NULL);


        fclose(fp);

        png_destroy_write_struct (&png_ptr, &info_ptr);
    }


    virtual Bitmap & display() {

        uint8_t length = width()/8;
        RenderContext rc(pool_registry.get(), width(), 1, _row_pointers[0]);

        for(uint32_t y = 0; y < height(); ++y) {

            for(int i = 0; i < length; ++i) {
                _row_pointers[y][i] = 0;
            }
            rc.advance(0, y, (uint8_t*)_row_pointers[y]);

            active().draw(rc);

            rc.finish();

            // PNG has the bits reversed in each byte and inverted. Not sure why...
            for(int i = 0; i < length; ++i) {
                // this insanity reverses the bit in each byte: http://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64BitsDiv
                _row_pointers[y][i] = (_row_pointers[y][i] * 0x0202020202ULL & 0x010884422010ULL) % 1023;

                // this inverts the display
                _row_pointers[y][i] = ~ _row_pointers[y][i];
            }

        }

        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TODO replace this section with ADisplay...

    Bitmap& clear() {
        active().clear();
        return *this;
    }

    virtual void invertDisplay(bool i) {
        active().invertDisplay(i);
    }

    virtual Bitmap& drawString(coord_t x, coord_t y, const string& str, color_t color) {
        active().drawString(x,y , str, color);
        return *this;
    }

    virtual Bitmap& drawString(coord_t x, coord_t y, const string& str, Font& font, color_t color) {
        active().drawString(x,y, str, font, color);
        return *this;
    }

    virtual Bitmap& setFont(Font* f) {
        active().setFont(f);
        return *this;
    }

    virtual Bitmap& appendChild(DisplayObject *obj) {
        active().appendChild(obj);
        return *this;
    }

    virtual RLEImage& drawImage(const RLEImageMessage& message) {
        return active().drawImage(message);
    }

    virtual RLEImage& drawImage() {
        return active().drawImage();
    }


    virtual Bitmap& drawPath(string string1) {
        active().drawPath(string1);
        return *this;
    }

    virtual Bitmap& drawLine(coord_t x0, coord_t y0, coord_t x1, coord_t y1, color_t color){
        active().drawLine(x0, y0, x1, y1, color);
        return *this;
    }

    virtual Bitmap& drawFastVLine(coord_t x, coord_t y, coord_t h, color_t color) {
        active().drawFastVLine(x, y, h, color);
        return *this;
    }

    virtual Bitmap& drawFastHLine(coord_t x, coord_t y, coord_t w, color_t color){
        active().drawFastHLine(x, y, w, color);
        return *this;
    }

    virtual Bitmap& drawRect(coord_t x, coord_t y, coord_t w, coord_t h, color_t color) {
        active().drawRect(x, y, w, h, color);
        return *this;
    }

    virtual Bitmap& fillRect(coord_t x, coord_t y, coord_t w, coord_t h, color_t color) {
        active().fillRect(x, y, w, h, color);
        return *this;
    }

    virtual Bitmap& fillCheckerboard(coord_t x, coord_t y, coord_t w, coord_t h,
            color_t color) {
        active().fillCheckerboard(x, y, w, h, color);
        return *this;
    }

    virtual Bitmap& fillScreen(color_t color) {
        active().fillScreen(color);
        return *this;
    }

    virtual Bitmap& drawCircle(coord_t x0, coord_t y0, coord_t Bitmap, color_t color) {
        active().drawCircle(x0, y0, Bitmap, color);
        return *this;
    }

    virtual Bitmap& fillCircle(coord_t x0, coord_t y0, coord_t Bitmap, color_t color){
        active().fillCircle(x0, y0, Bitmap, color);
        return *this;
    }

    virtual Bitmap& drawTriangle(coord_t x0, coord_t y0, coord_t x1, coord_t y1, coord_t x2, coord_t y2, color_t color) {
        active().drawTriangle(x0, y0, x1, y1, x2, y2, color);
        return *this;
    }

    virtual Bitmap& fillTriangle(coord_t x0, coord_t y0, coord_t x1, coord_t y1, coord_t x2, coord_t y2, color_t color) {
        active().fillTriangle(x0, y0, x1, y1, x2, y2, color);
        return *this;
    }

    virtual Bitmap& drawRoundRect(coord_t x0, coord_t y0, coord_t w, coord_t h, coord_t radius, color_t color) {
        active().drawRoundRect(x0, y0, w, h, radius, color);
        return *this;
    }

    virtual Bitmap& fillRoundRect(coord_t x0, coord_t y0, coord_t w, coord_t h, coord_t radius, color_t color) {
        active().fillRoundRect(x0, y0, w, h, radius, color);
        return *this;
    }

    virtual Bitmap& drawBitmap(coord_t x, coord_t y, const uint8_t *bitmap, coord_t w, coord_t h, color_t color) {
        active().drawBitmap(x, y, bitmap, w, h, color);
        return *this;
    }

    virtual Bitmap& drawChar(coord_t x, coord_t y, unsigned char c, color_t color, color_t bg, uint8_t size) {
        active().drawChar(x, y, c, color, bg, size);
        return *this;
    }

    virtual Bitmap& setRotation(coord_t Bitmap) {
        active().setRotation(Bitmap);
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
//    virtual Bitmap& setCursor(coord_t x, coord_t y) {
//    }
//
//    virtual Bitmap& setTextColor(color_t c);
//
//    virtual Bitmap& setTextColor(color_t c, color_t bg);
//
//    virtual Bitmap& setTextSize(uint8_t s);
//
//    virtual Bitmap& setTextWrap(bool w);

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

};

#endif /* _Bitmap_h */