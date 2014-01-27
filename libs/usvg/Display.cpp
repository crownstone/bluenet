
#include "Display.h"



Display::~Display() {}
RectDisplayObject::~RectDisplayObject() {
//    _height = _width = 0;

}


DisplayList::~DisplayList() {
    cleanup();
}
