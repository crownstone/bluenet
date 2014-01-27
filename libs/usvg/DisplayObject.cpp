
#include "DisplayObject.h"

RenderContext::~RenderContext() {
//    for(auto item : context) {
//        item.first->cleanup(*this, item.second);
//    }


    if (state_owner) {
        state_owner->cleanup(*this, 0);
    }
}


DisplayObject::~DisplayObject() {
    _x = _y = 0;
    _parent = _nextSibling = NULL;
}

DisplayObjectContainer::~DisplayObjectContainer() {
    clear();
}


Group::~Group() {
    _transform->~PMatrix();
    getPool()->release(sizeof(PMatrix), _transform);
    _transform = 0;
}

Checkerboard::~Checkerboard() {
    _width = _height = 0;
}



#ifdef USE_RENDER_CONTEXT
void Checkerboard::draw(RenderContext& ctx) {
    coord_t x = std::max(ctx.start_x, getX());
    coord_t y = std::max(ctx.start_y, getY());
    coord_t end_x = std::min(ctx.start_x + ctx.width, getX() + getWidth());
    coord_t end_y = std::min(ctx.start_y + ctx.height, getY() + getHeight());

    for(; y < end_y; ++y) {
        for(; x < end_x; ++x) {
            bool on = ((y % 2) == 0) ? ((x % 2) == 0) : ((x % 2) == 1);  // x=0, y=0 -> 0; x=1, y=0 -> 1; x = 0, y = 1 -> 1; x = 1, y = 1 -> 0.

            if (on) ctx.setPixel(x,y, _color);
        }
    }

}

void Rectangle::draw(RenderContext& ctx) {
    int startX = std::max(ctx.start_x,this->getX());
    int startY = std::max(ctx.start_y,this->getY());

    int endX = std::min(ctx.start_x + ctx.width,this->getX() + this->getWidth());
    int endY = std::min(ctx.start_y + ctx.height,this->getY() + this->getHeight());

    for ( int x = startX ; x < endX ; ++x ) {
        for ( int y = startY ; y < endY ; ++y ) {
            ctx.setPixel(x, y, 1);
        }
    }
}

Rectangle::~Rectangle(){
    _width = _height = 0;
}

void Line::draw(RenderContext& ctx){
    coord_t startX = ctx.start_x;
    coord_t startY = ctx.start_y;

    coord_t endX = ctx.start_x + ctx.width;
    coord_t endY = ctx.start_y + ctx.height;

    if ( (y0 < startY && (y0 + dy) < startY) || (y0 >= endY && (y0 + dy) >= endY) ) { return; }

    if ( dx == 0 ) {
        for ( int y = startY ; y < endY ; ++y ) {
            ctx.setPixel(x0,y,1);
        }
    } else if ( dy == 0 ) {
        if ( y0 == startY ) {
            coord_t x1 = x0;
            coord_t x2 = x0 + dx;
            if ( x1 > x2 ) {std::swap(x1,x2);}
            for ( int x = x1 ; x < x2 ; ++x ) {
                ctx.setPixel(x,startY,1);
            }
        }
    } else {

        float t0 = (startY - y0) / float(dy);
//        float t1 = (startY - y0) / float(dy);
        float t1 = y0 > startY ? t0 + (1.0f / float(dy)) : t0 - (1.0f  / float(dy));

        coord_t x1 = x0 + int(dx*t0);
        coord_t x2 = x0 + int(dx*t1);
        if ( x1 > x2 ){std::swap(x1,x2);}
        if ( x1 == x2 ) { x2 += 1; }
        x1 = std::max(x1,std::min(x0,coord_t(x0+dx)));
        x2 = std::min(x2,coord_t(std::max(x0,coord_t(x0+dx))+1));
        for ( int x = x1 ; x < x2 ; ++ x ) {
            ctx.setPixel(x,startY,1);
        }
    }


}

Line::~Line(){
    dx = dy = x0 = y0 = 0;
}

#else /* USE_RENDER_CONTEXT */

void Checkerboard::draw(RenderContext& ctx) {
    coord_t x = std::max(ctx.start_x, getX());
    coord_t y = std::max(ctx.start_y, getY());
    coord_t end_x = std::min(ctx.start_x + ctx.width, getX() + getWidth());
    coord_t end_y = std::min(ctx.start_y + ctx.height, getY() + getHeight());

    uint16_t off = x - ctx.start_x + ctx.width * (y-ctx.start_y) + ctx.stride * (y- ctx.start_y);
    uint8_t mask = ctx.mask + off % 8;
    uint8_t* buf = ctx.buffer + off/8;
    uint8_t b = *buf;

    for(; y < end_y; ++y) {
        for(; x < end_x; ++x) {
            bool on = x % 2 + y % 2 == 0;  // x=0, y=0 -> 0; x=1, y=0 -> 1; x = 0, y = 1 -> 1; x = 1, y = 1 -> 0.
            if (on) {
                if (_color) {
                     b |= 1 << mask;
                } else {
                    b &= ~(1 << mask);
                }
            }

            if (++mask == 8) {
                *(buf++) = b;
                mask = 0;
            }

        }
        b += ctx.stride;
        mask = 0;
    }
    *(buf++) = b;

}



#endif /* USE_RENDER_CONTEXT */