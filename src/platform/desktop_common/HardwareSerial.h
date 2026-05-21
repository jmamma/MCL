// HardwareSerial.h — desktop shim. Forwards Arduino-style Serial.print/println
// calls to stderr so MCL's debug output is still visible from the JUCE host.
#pragma once

#include "Stream.h"
#include <stdio.h>
#include <string.h>

class HardwareSerial : public Stream {
public:
    void begin(unsigned long /*baud*/) {}
    void end() {}

    int available() override { return 0; }
    int read()      override { return -1; }
    int peek()      override { return -1; }
    size_t write(uint8_t b) override {
        fputc(b, stderr);
        return 1;
    }
    size_t write(const uint8_t* buf, size_t n) override {
#if defined(PLATFORM_WASM)
        char chunk[129];
        size_t pos = 0;
        while (pos < n) {
            size_t take = n - pos;
            if (take >= sizeof(chunk))
                take = sizeof(chunk) - 1;
            memcpy(chunk, buf + pos, take);
            chunk[take] = '\0';
            fputs(chunk, stderr);
            pos += take;
        }
#else
        for (size_t i = 0; i < n; ++i) fputc(buf[i], stderr);
#endif
        return n;
    }
    operator bool() const { return true; }
};

extern HardwareSerial Serial;
extern HardwareSerial Serial1;
extern HardwareSerial Serial2;
