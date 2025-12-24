#pragma once

// oled.h stub for desktop builds

#include <stdint.h>

class OLEDClass {
public:
    void begin() {}
    void clear() {}
    void display() {}
    void setPixel(int x, int y, uint8_t color) {}
    void drawLine(int x0, int y0, int x1, int y1, uint8_t color) {}
    void drawRect(int x, int y, int w, int h, uint8_t color) {}
    void fillRect(int x, int y, int w, int h, uint8_t color) {}
    void drawBitmap(int x, int y, const uint8_t* bitmap, int w, int h, uint8_t color) {}
    void setCursor(int x, int y) {}
    void print(const char* str) {}
    void print(int n) {}
    void println(const char* str) {}
    void println() {}
    uint8_t* getBuffer() { return nullptr; }
};

extern OLEDClass oled;
