//
// Created by Christopher Mason on 5/12/13.
// Copyright (c) 2013 Mason. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//



#ifndef __Debug_H_
#define __Debug_H_

#include <iostream>
#include "Stream.h"


#define NRF51_CRASH(x) \
    std::cerr << x << std::endl; \
    abort();


class Debug_class : public Stream{
public:
    virtual ~Debug_class();
    virtual int available() { return (bool)std::cin; }
    virtual int read() { char c; std::cin >> c; return c; }
    virtual int peek() { return std::cin.peek(); }
    virtual void flush() {
        std::cout.flush();
    }
    virtual size_t write(uint8_t c) {std::cout << (char)c;
        return 1;
    }
    virtual size_t write(const unsigned char *buffer, size_t size) {
        std::cout.write((char*)buffer, size);
        return size;
    }

    using Print::write;
};

extern Debug_class Debug;

#endif //__Debug_H_
