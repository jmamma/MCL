// HardwareSerial.h — desktop shim. Forwards Arduino-style Serial.print/println
// calls to stderr so MCL's debug output is still visible from the JUCE host.
#pragma once

#include "Stream.h"
#include <cstdio>

class HardwareSerial : public Stream {
public:
    void begin(unsigned long /*baud*/) {}
    void end() {}

    int available() override { return 0; }
    int read()      override { return -1; }
    int peek()      override { return -1; }
    size_t write(uint8_t b) override {
        std::fputc(b, stderr);
        return 1;
    }
    size_t write(const uint8_t* buf, size_t n) override {
        return std::fwrite(buf, 1, n, stderr);
    }
    operator bool() const { return true; }
};

extern HardwareSerial Serial;
extern HardwareSerial Serial1;
extern HardwareSerial Serial2;
