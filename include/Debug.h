
#ifndef _Debug_h
#define _Debug_h

#include <stdint.h>
#include <stddef.h>
#include "Stream.h"

#define NRF51_CRASH(x) __asm("BKPT"); \
while (1) {}


class DebugClass : public Stream {
  public:
    void begin(long) { /* TODO: call a function that tries to wait for enumeration */ };
    void end() { /* TODO: flush output and shut down USB port */ };
    virtual int available() { return 0; }
    virtual int read() { return 0; }
    virtual int peek() { return 0; }
    virtual void flush() { }
    virtual size_t write(uint8_t c) { return 1; }
    virtual size_t write(const uint8_t *buffer, size_t size) { return size; }

    //using Print::write;
    void send_now(void) { flush(); };



};

extern DebugClass Debug;

#endif /* _Debug_h */
