// Print.h — desktop shim.
//
// MCL's Stream-derived classes (MidiUartClass, debug log) override write(uint8_t).
// We give them the standard Arduino-style Print interface that forwards
// print/println convenience overloads back to write(). Output destination is
// controlled by each subclass (Serial routes to stderr; MidiUart routes to
// a ring buffer; etc).
#pragma once

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstring>

class Print {
public:
    virtual ~Print() = default;
    virtual size_t write(uint8_t) = 0;
    virtual size_t write(const uint8_t* buf, size_t size) {
        size_t n = 0;
        while (size--) { if (write(*buf++)) ++n; else break; }
        return n;
    }
    size_t write(const char* str) {
        return str ? write(reinterpret_cast<const uint8_t*>(str), std::strlen(str)) : 0;
    }
    size_t write(const char* buf, size_t size) {
        return write(reinterpret_cast<const uint8_t*>(buf), size);
    }

    size_t print(const char* s) { return write(s); }
    size_t print(char c)        { return write(static_cast<uint8_t>(c)); }

    size_t print(int n, int base = 10) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), base == 16 ? "%x" : "%d", n);
        return write(buf);
    }
    size_t print(unsigned int n, int base = 10) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), base == 16 ? "%x" : "%u", n);
        return write(buf);
    }
    size_t print(long n, int base = 10) {
        char buf[24];
        std::snprintf(buf, sizeof(buf), base == 16 ? "%lx" : "%ld", n);
        return write(buf);
    }
    size_t print(unsigned long n, int base = 10) {
        char buf[24];
        std::snprintf(buf, sizeof(buf), base == 16 ? "%lx" : "%lu", n);
        return write(buf);
    }
    size_t print(double n, int digits = 2) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.*f", digits, n);
        return write(buf);
    }

    size_t println()                                   { return write("\r\n"); }
    template <class T> size_t println(T v)             { size_t n = print(v); return n + println(); }
    template <class T> size_t println(T v, int base)   { size_t n = print(v, base); return n + println(); }

    virtual void flush() {}
};
