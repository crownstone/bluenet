#ifndef Print_h
#define Print_h

#include <inttypes.h>
#include <string.h>
#include <stdio.h> // for size_t - gives sprintf and other stuff to all sketches & libs
//#include "core_id.h"
#include "WString.h"
//#include "Printable.h"

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2
#define BYTE 0

class __FlashStringHelper;

class Print
{
  public:
	Print() : write_error(0) {}
	virtual size_t write(uint8_t b) = 0;
	size_t write(const char *str)			{ return write((const uint8_t *)str, strlen(str)); }
	virtual size_t write(const uint8_t *buffer, size_t size);
	//size_t print(const String &s);
	size_t print(char c)				{ return write((uint8_t)c); }
	size_t print(const char s[])			{ return write(s); }
	size_t print(const __FlashStringHelper *f)	{ return write((const char *)f); }

	size_t print(uint8_t b)				{ return printNumber(b, 10, 0); }
	size_t print(int n)				{ return print((long)n); }
	size_t print(unsigned int n)			{ return printNumber(n, 10, 0); }
	size_t print(long n);
	size_t print(unsigned long n)			{ return printNumber(n, 10, 0); }

	size_t print(unsigned char n, int base)		{ return printNumber(n, base, 0); }
	size_t print(int n, int base)			{ return (base == 10) ? print(n) : printNumber(n, base, 0); }
	size_t print(unsigned int n, int base)		{ return printNumber(n, base, 0); }
	size_t print(long n, int base)			{ return (base == 10) ? print(n) : printNumber(n, base, 0); }
	size_t print(unsigned long n, int base)		{ return printNumber(n, base, 0); }

	size_t print(double n, int digits = 2)		{ return printFloat(n, digits); }
	//size_t print(const Printable &obj)		{ return obj.printTo(*this); }
	size_t println(void);
	//size_t println(const String &s)			{ return print(s) + println(); }
	size_t println(char c)				{ return print(c) + println(); }
	size_t println(const char s[])			{ return print(s) + println(); }
	size_t println(const __FlashStringHelper *f)	{ return print(f) + println(); }

	size_t println(uint8_t b)			{ return print(b) + println(); }
	size_t println(int n)				{ return print(n) + println(); }
	size_t println(unsigned int n)			{ return print(n) + println(); }
	size_t println(long n)				{ return print(n) + println(); }
	size_t println(unsigned long n)			{ return print(n) + println(); }

	size_t println(unsigned char n, int base)	{ return print(n, base) + println(); }
	size_t println(int n, int base)			{ return print(n, base) + println(); }
	size_t println(unsigned int n, int base)	{ return print(n, base) + println(); }
	size_t println(long n, int base)		{ return print(n, base) + println(); }
	size_t println(unsigned long n, int base)	{ return print(n, base) + println(); }

	size_t println(double n, int digits = 2)	{ return print(n, digits) + println(); }
	//size_t println(const Printable &obj)		{ return obj.printTo(*this) + println(); }
	int getWriteError() { return write_error; }
	void clearWriteError() { setWriteError(0); }
	size_t printNumber(unsigned long n, uint8_t base, uint8_t sign);
  protected:
	void setWriteError(int err = 1) { write_error = err; }
  private:
	char write_error;
	size_t printFloat(double n, uint8_t digits);
};


#endif
